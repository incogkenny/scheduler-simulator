#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdarg.h>
#include <unistd.h>

#include "coursework.h"
#include "linkedlist.h"

typedef struct
{
    int pids[SIZE_OF_PROCESS_TABLE];
    int top;
} Stack;

#define STACK_INITILIASER {.top = -1};

void *processTable[SIZE_OF_PROCESS_TABLE];
Stack pidPool = STACK_INITILIASER;
LinkedList ReadyQueue[NUMBER_OF_PRIORITY_LEVELS];
LinkedList TerminatedQueue = LINKED_LIST_INITIALIZER;
LinkedList IOQueue[NUMBER_OF_IO_DEVICES];
int boosting = true, io_sim = true;
pthread_mutex_t print_lock, readyQ_lock, terminatedQ_lock, pool_lock, table_lock, booster_lock, io_lock;
sem_t generator_sem, simulator_sem, terminator_sem;
void* generator();
void* simulator();
void* terminator();
void* booster();
void* io_simulator();

int main() {
    pthread_t thread_generator, thread_simulator, thread_terminator, thread_booster, thread_ioSim;
    // Create mutexes and semaphores
    pthread_mutex_init(&print_lock, 0);
    pthread_mutex_init(&readyQ_lock, 0);
    pthread_mutex_init(&terminatedQ_lock, 0);
    pthread_mutex_init(&pool_lock, 0);
    pthread_mutex_init(&table_lock, 0);
    pthread_mutex_init(&booster_lock, 0);
    pthread_mutex_init(&io_lock, 0);

    sem_init(&generator_sem, 0, MAX_CONCURRENT_PROCESSES);
    sem_init(&simulator_sem,0,0);
    sem_init(&terminator_sem, 0, 0);

    // Create threads for generator, simulator and terminator
    pthread_create(&thread_generator, NULL, generator, NULL);
    pthread_create(&thread_simulator, NULL, simulator, NULL);
    pthread_create(&thread_terminator, NULL, terminator, NULL);
    pthread_create(&thread_booster, NULL, booster, NULL);
    pthread_create(&thread_ioSim, NULL, io_simulator, NULL);

    // Wait for the threads to complete
    pthread_join(thread_generator, NULL);
    pthread_join(thread_simulator, NULL);
    pthread_join(thread_terminator, NULL);
    pthread_join(thread_booster, NULL);
    pthread_join(thread_ioSim, NULL);

    pthread_mutex_destroy(&print_lock);
    pthread_mutex_destroy(&readyQ_lock);
    pthread_mutex_destroy(&terminatedQ_lock);
    pthread_mutex_destroy(&pool_lock);
    pthread_mutex_destroy(&table_lock);
    pthread_mutex_destroy(&booster_lock);
    pthread_mutex_destroy(&io_lock);

    sem_destroy(&generator_sem);
    sem_destroy(&simulator_sem);
    sem_destroy(&terminator_sem);

    return 0;
}

// Utility Functions

//  Returns size of a Queue
int getQueueSize(LinkedList *pList) {
    int size = 0;
    Element *current = pList->pHead; // Start at the head of the list

    while (current != NULL) {
        size++; // Increment the size for each element
        current = current->pNext; // Move to the next element
    }

    return size;
}

void safe_printf(const char *format, ...) {


    // Lock the mutex before calling vprintf
    pthread_mutex_lock(&print_lock);
    va_list args;
    // Start variadic argument handling
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    // Unlock the mutex after calling vprintf
    pthread_mutex_unlock(&print_lock);
}

void pop(Stack *stk)
{
    if (stk->top != -1)
    {
        stk->top--;
    }
}
void push(Stack *stk, int pid)
{
    if (stk->top < SIZE_OF_PROCESS_TABLE - 1)
    {
        stk->top++;
        stk->pids[stk->top] = pid;
    }
}

int peek(Stack *stk)
{
    if (stk->top == -1)
    {
        return -1;
    }
    return stk->pids[stk->top];
}

char* get_process_state(int iState){
    switch(iState){
        case 1:
            return "READY";
        case 2:
            return "RUNNING";
        case 3:
            return "BLOCKED";
        case 4:
            return "TERMINATED";
    }
}

//GENERATE PROCESSES AND ADD THEM TO QUEUE
void* generator() {

    pthread_mutex_lock(&io_lock);
    for(int i = 0; i < NUMBER_OF_IO_DEVICES; i++){
        LinkedList temp_queue = LINKED_LIST_INITIALIZER;
        IOQueue[i] = temp_queue;
    }
    pthread_mutex_unlock(&io_lock);

    for (int process_count = 0; process_count < NUMBER_OF_PROCESSES; process_count++){
        sem_wait(&generator_sem);
        // Creates and prints current process
        Process *current_process;
        if (peek(&pidPool) != -1)
        {
            current_process = generateProcess(peek(&pidPool));
        }
        else
        {
            current_process = generateProcess(process_count);
        }
        // GENERATOR - CREATED
        safe_printf("GENERATOR - CREATED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",current_process->iPID,current_process->iPriority, current_process->iBurstTime, current_process->iRemainingBurstTime);
        pthread_mutex_lock(&table_lock);
        processTable[current_process->iPID] = current_process;
        printf("GENERATOR - ADDED TO TABLE: [PID = %d, Priority = %d, Initial BurstTime = %d, Remaining BurstTime = %d]\n", current_process->iPID, current_process->iPriority, current_process->iBurstTime, current_process->iRemainingBurstTime);
        pthread_mutex_unlock(&table_lock);

        // Adds process to oReadyQueue and prints that process was added to ready queue and that process is admitted
        pthread_mutex_lock(&readyQ_lock);
        addLast(current_process, &ReadyQueue[current_process->iPriority]);
        safe_printf("QUEUE - ADDED: [Queue = READY %d, Size = %d, PID = %d, Priority = %d]\n", current_process->iPriority, getQueueSize(&ReadyQueue[current_process->iPriority]), current_process->iPID, current_process->iPriority);
        pthread_mutex_unlock(&readyQ_lock);

        pthread_mutex_lock(&pool_lock);
        pop(&pidPool);
        pthread_mutex_unlock(&pool_lock);


        // GENERATOR - ADMITTED
        safe_printf("GENERATOR - ADMITTED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n", current_process->iPID,current_process->iPriority, current_process->iBurstTime, current_process->iRemainingBurstTime);
        sem_post(&simulator_sem);
    }
    safe_printf("GENERATOR: Finished\n");
}

//SIMULATE PROCESSES AND ADD THEM TO CORRESPONDING QUEUE
void* simulator() {
    int terminated_count = 0;

    while(terminated_count != NUMBER_OF_PROCESSES){

        sem_wait(&simulator_sem); // Make sure there is something to simulate

        int queue_num = -1;

        pthread_mutex_lock(&readyQ_lock);
        for (int i = 0; i < NUMBER_OF_PRIORITY_LEVELS; i++)
        {
            if (getQueueSize(&ReadyQueue[i]) > 0)
            {
                queue_num = i;
                break;
            }
        }
        pthread_mutex_unlock(&readyQ_lock);

        pthread_mutex_lock(&readyQ_lock);
        Process *current_process = removeFirst(&ReadyQueue[queue_num]);
        safe_printf("QUEUE - REMOVED: [Queue = READY %d, Size = %d, PID = %d, Priority = %d]\n",
                    current_process->iPriority, getQueueSize(&ReadyQueue[queue_num]), current_process->iPID,
                    current_process->iPriority);
        pthread_mutex_unlock(&readyQ_lock);

        // run process
        if(current_process->iPriority < NUMBER_OF_PRIORITY_LEVELS / 2) {
            runNonPreemptiveProcess(current_process, true);
            safe_printf(
                "SIMULATOR - CPU 0: FCFS [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
                current_process->iPID, current_process->iPriority, current_process->iBurstTime,
                current_process->iRemainingBurstTime);
        }
        else {
            runPreemptiveProcess(current_process, true);
            safe_printf(
                "SIMULATOR - CPU 0: RR [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
                current_process->iPID, current_process->iPriority, current_process->iBurstTime,
                current_process->iRemainingBurstTime);
        }

        // Decide process outcome
        if(current_process->iState == READY){
            pthread_mutex_lock(&readyQ_lock);
            addLast(current_process, &ReadyQueue[current_process->iPriority]);
            safe_printf("QUEUE - ADDED: [Queue = READY %d, Size = %d, PID = %d, Priority = %d]\n",
                        current_process->iPriority, getQueueSize(&ReadyQueue[queue_num]), current_process->iPID,
                        current_process->iPriority);
            safe_printf("SIMULATOR - CPU 0 - READY: [PID = %d, Priority = %d]\n", current_process->iPID, current_process->iPriority);
            pthread_mutex_unlock(&readyQ_lock);
            sem_post(&simulator_sem);
        }

        if(current_process->iState == BLOCKED){
            safe_printf("SIMULATOR - CPU 0 - I/O BLOCKED: [PID = %d, Priority = %d, Device = %d]\n",
                        current_process->iPID, current_process->iPriority, current_process->iDeviceID);
            pthread_mutex_lock(&io_lock);
            addLast(current_process, &IOQueue[current_process->iDeviceID]);
            safe_printf("QUEUE - ADDED: [Queue = I/O %d, Size = %d, PID = %d, Priority = %d]\n",
                        current_process->iDeviceID, getQueueSize(&IOQueue[current_process->iDeviceID]), current_process->iPID, current_process->iPriority);
            sem_post(&simulator_sem);
            pthread_mutex_unlock(&io_lock);
        }

        if(current_process->iState == TERMINATED){
            pthread_mutex_lock(&terminatedQ_lock);
            safe_printf("SIMULATOR - CPU 0 - TERMINATED: [PID = %d, ResponseTime = %ld, TurnAroundTime = %ld]\n",
                        current_process->iPID,
                        getDifferenceInMilliSeconds(current_process->oTimeCreated, current_process->oFirstTimeRunning),
                        getDifferenceInMilliSeconds(current_process->oTimeCreated, current_process->oLastTimeRunning));
            addLast(current_process, &TerminatedQueue);
            terminated_count++;
            safe_printf("QUEUE - ADDED: [Queue = TERMINATED, Size = %d, PID = %d, Priority = %d]\n",
                        getQueueSize(&TerminatedQueue), current_process->iPID, current_process->iPriority);
            pthread_mutex_unlock(&terminatedQ_lock);
            sem_post(&terminator_sem);
        }
    }

    printf("SIMULATOR: Finished\n");
    pthread_mutex_lock(&io_lock);
    io_sim = false;
    pthread_mutex_unlock(&io_lock);
    pthread_mutex_lock(&booster_lock);
    boosting = false;
    pthread_mutex_unlock(&booster_lock);
}

// TERMINATE PROCESSES AND CLEAR MEMORY ASSOCIATED
void* terminator() {
    int no_terminated = 0;
    double response_times = 0, turnaround_times = 0;

    while(no_terminated != NUMBER_OF_PROCESSES){
        sem_wait(&terminator_sem);

        pthread_mutex_lock(&terminatedQ_lock); // Lock terminated Queue
        Process *current_process = removeFirst(&TerminatedQueue);
        no_terminated++;
        response_times +=
                getDifferenceInMilliSeconds(current_process->oTimeCreated, current_process->oFirstTimeRunning);
        turnaround_times += getDifferenceInMilliSeconds(current_process->oTimeCreated,
                                                        current_process->oLastTimeRunning);
        safe_printf("QUEUE - REMOVED: [Queue = TERMINATED, Size = %d, PID = %d, Priority = %d]\n",
                    getQueueSize(&TerminatedQueue), current_process->iPID, current_process->iPriority);

        safe_printf("TERMINATION DAEMON - CLEARED: [#iTerminated = %d, PID = %d, Priority = %d]\n", no_terminated,
                    current_process->iPID, current_process->iPriority);
        pthread_mutex_unlock(&terminatedQ_lock); // Unlock terminated Queue

        pthread_mutex_lock(&pool_lock);
        push(&pidPool, current_process->iPID);
        pthread_mutex_unlock(&pool_lock);

        pthread_mutex_lock(&table_lock);
        processTable[current_process->iPID] = NULL;
        pthread_mutex_unlock(&table_lock);

        destroyProcess(current_process);
        sem_post(&generator_sem);// Signal that a new process can be created
    }
    double mean_response = response_times/NUMBER_OF_PROCESSES;
    double mean_turnaround = turnaround_times/NUMBER_OF_PROCESSES;

    safe_printf("TERMINATION DAEMON: Finished\n");
    safe_printf("TERMINATION DAEMON: [Average Response Time = %lf, Average Turn Around Time = %lf]\n",
        mean_response, mean_turnaround);
}

void* booster() {
    safe_printf("BOOSTER DAEMON: Created\n");

    while(1) {
        pthread_mutex_lock(&booster_lock);
        if(!boosting){
            pthread_mutex_unlock(&booster_lock);
            break;
        }
        pthread_mutex_unlock(&booster_lock);
        Process *current_process;

        pthread_mutex_lock(&readyQ_lock);
        for (int i = NUMBER_OF_PRIORITY_LEVELS / 2 + 1; i < NUMBER_OF_PRIORITY_LEVELS; i++) {
            while (1) {
                current_process = removeFirst(&ReadyQueue[i]);
                if (current_process != NULL) {
                    safe_printf("QUEUE - REMOVED: [Queue = %s %d, Size = %d, PID = %d, Priority = %d]\n",
                                get_process_state(current_process->iState), current_process->iPriority,
                                getQueueSize(&ReadyQueue[current_process->iPriority]), current_process->iPID,
                                current_process->iPriority);
                    safe_printf(
                        "BOOSTER DAEMON: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d] => Boosted to Level %d\n",
                        current_process->iPID, current_process->iPriority, current_process->iBurstTime,
                        current_process->iRemainingBurstTime, NUMBER_OF_PRIORITY_LEVELS / 2);
                    addLast(current_process, &ReadyQueue[(NUMBER_OF_PRIORITY_LEVELS / 2)]);
                    safe_printf("QUEUE - ADDED: [Queue = %s %d, Size = %d, PID = %d, Priority = %d]\n", "READY",
                                NUMBER_OF_PRIORITY_LEVELS / 2,
                                getQueueSize(&ReadyQueue[(NUMBER_OF_PRIORITY_LEVELS / 2)]), current_process->iPID,
                                current_process->iPriority);
                } else break;
            }
        }
        pthread_mutex_unlock(&readyQ_lock);
        usleep(BOOST_INTERVAL * 1000);
    }
    safe_printf("BOOSTER DAEMON: Finished\n");
    }

void* io_simulator(){
    while(1){
        pthread_mutex_lock(&io_lock);
        if(!io_sim){
            pthread_mutex_unlock(&io_lock);
            break;
        }
        Process *cP;
        for(int i = 0; i < NUMBER_OF_IO_DEVICES; i++){
            while(cP = removeFirst(&(IOQueue[i]))){
                safe_printf("QUEUE - REMOVED: [Queue = I/O %d, Size = %d, PID = %d, Priority = %d]\n",
                            i, getQueueSize(&IOQueue[i]), cP->iPID, cP->iPriority);
                unblockProcess(cP);
                safe_printf("I/O DAEMON - UNBLOCKED: [PID = %d, Priority = %d]\n", cP->iPID, cP->iPriority);
                pthread_mutex_lock(&readyQ_lock);
                addFirst(cP, &ReadyQueue[cP->iPriority]);
                safe_printf("QUEUE - ADDED: [Queue = %s %d, Size = %d, PID = %d, Priority = %d]\n",
                            get_process_state(cP->iState),cP->iPriority, getQueueSize(&ReadyQueue[cP->iPriority]), cP->iPID, cP->iPriority);
                pthread_mutex_unlock(&readyQ_lock);
            }
        }
        pthread_mutex_unlock(&io_lock);
        usleep(IO_DAEMON_INTERVAL*1000);
    }
    safe_printf("I/O DAEMON: Finished\n");
}

