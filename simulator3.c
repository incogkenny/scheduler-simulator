#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdarg.h>

#include "coursework.h"
#include "linkedlist.h"


LinkedList oReadyQueue = LINKED_LIST_INITIALIZER;
LinkedList oTerminatedQueue = LINKED_LIST_INITIALIZER;
double response_times = 0, turnaround_times = 0;

int readyQ_length = 0;
int terminated_count = 0;
int terminatedQ_length = 0;
pthread_mutex_t print_lock, readyQ_lock, terminatedQ_lock;
sem_t generator_sem, simulator_sem, terminator_sem;

// Utility Functions

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

//GENERATE PROCESSES AND ADD THEM TO QUEUE
void *generator() {
    int process_count;
    for (process_count = 0; process_count< NUMBER_OF_PROCESSES; process_count++){
        sem_wait(&generator_sem);
        // Creates and prints current process
        Process *current_process = generateProcess(process_count);
        // GENERATOR - CREATED
        printf("GENERATOR - CREATED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",current_process->iPID,current_process->iPriority, current_process->iBurstTime, current_process->iRemainingBurstTime);


        // Adds process to oReadyQueue and prints that process was added to ready queue and that process is admitted
        pthread_mutex_lock(&readyQ_lock);
        addLast(current_process, &oReadyQueue);
        readyQ_length++;

        // QUEUE - ADDED
        safe_printf("QUEUE - ADDED: [Queue = READY, Size = %d, PID = %d, Priority = %d]\n", readyQ_length, current_process->iPID, current_process->iPriority);

        if(process_count==MAX_CONCURRENT_PROCESSES)
        {
            sem_post(&simulator_sem);
        }

        // GENERATOR - ADMITTED
        safe_printf("GENERATOR - ADMITTED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n", current_process->iPID,current_process->iPriority, current_process->iBurstTime, current_process->iRemainingBurstTime);
        pthread_mutex_unlock(&readyQ_lock);
    }
    safe_printf("GENERATOR: Finished\n");
}

//SIMULATE PROCESSES AND ADD THEM TO CORRESPONDING QUEUE
void *simulator() {

    sem_wait(&simulator_sem);
    while(terminated_count<NUMBER_OF_PROCESSES){

        pthread_mutex_lock(&readyQ_lock);
        Process *current_process = getHead(oReadyQueue)->pData;
        removeFirst(&oReadyQueue);
        readyQ_length--;
        safe_printf("QUEUE - REMOVED: [Queue = READY, Size = %d, PID = %d, Priority = %d]\n", readyQ_length, current_process->iPID, current_process->iPriority);
        pthread_mutex_unlock(&readyQ_lock);

        // run process
        runPreemptiveProcess(current_process, false);
        safe_printf("SIMULATOR - CPU 0: RR [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n", current_process->iPID, current_process->iPriority, current_process->iBurstTime, current_process->iRemainingBurstTime);

        if(current_process->iState == READY){
            pthread_mutex_lock(&readyQ_lock);
            addLast(current_process, &oReadyQueue);
            readyQ_length++;
            safe_printf("QUEUE - ADDED: [Queue = READY, Size = %d, PID = %d, Priority = %d]\n", readyQ_length, current_process->iPID, current_process->iPriority);
            safe_printf("SIMULATOR - CPU 0 - READY: [PID = %d, Priority = %d]\n", current_process->iPID, current_process->iPriority);
            pthread_mutex_unlock(&readyQ_lock);
            sem_post(&simulator_sem);
        }

        if(current_process->iState == TERMINATED){
            pthread_mutex_lock(&terminatedQ_lock);
            safe_printf("SIMULATOR - CPU 0 - TERMINATED: [PID = %d, ResponseTime = %ld, TurnAroundTime = %ld]\n", current_process->iPID, getDifferenceInMilliSeconds(current_process->oTimeCreated, current_process->oFirstTimeRunning), getDifferenceInMilliSeconds(current_process->oTimeCreated, current_process->oLastTimeRunning));
            addLast(current_process, &oTerminatedQueue);
            terminatedQ_length++;
            terminated_count++;
            safe_printf("QUEUE - ADDED: [Queue = TERMINATED, Size = %d, PID = %d, Priority = %d]\n", terminatedQ_length, current_process->iPID, current_process->iPriority);
            pthread_mutex_unlock(&terminatedQ_lock);
            sem_post(&terminator_sem);
        }
    }

    printf("SIMULATOR: Finished\n");
}

// TERMINATE PROCESSES AND CLEAR MEMORY ASSOCIATED
void *terminator() {
    int no_terminated = 0;

    while(no_terminated!=NUMBER_OF_PROCESSES){
        sem_wait(&terminator_sem);

        pthread_mutex_lock(&terminatedQ_lock); // Lock terminated Queue
        Process *current_process = removeFirst(&oTerminatedQueue);
        terminatedQ_length--;
        no_terminated++;
        response_times += getDifferenceInMilliSeconds(current_process->oTimeCreated, current_process->oFirstTimeRunning);
        turnaround_times += getDifferenceInMilliSeconds(current_process->oTimeCreated, current_process->oLastTimeRunning);
        safe_printf("QUEUE - REMOVED: [Queue = TERMINATED, Size = %d, PID = %d, Priority = %d]\n", terminatedQ_length, current_process->iPID, current_process->iPriority);

        safe_printf("TERMINATION DAEMON - CLEARED: [#iTerminated = %d, PID = %d, Priority = %d]\n", no_terminated, current_process->iPID, current_process->iPriority);
        destroyProcess(current_process);
        pthread_mutex_unlock(&terminatedQ_lock); // Unlock terminated Queue


        sem_post(&generator_sem); // Signal that a new process can be created

        if (no_terminated >= NUMBER_OF_PROCESSES) {
            break;
        }
    }
    double mean_response = response_times/NUMBER_OF_PROCESSES;
    double mean_turnaround = turnaround_times/NUMBER_OF_PROCESSES;

    printf("TERMINATION DAEMON: Finished\n");
    printf("TERMINATION DAEMON: [Average Response Time = %lf, Average Turn Around Time = %lf]\n", mean_response, mean_turnaround);
}


int main() {
    pthread_t thread_generator, thread_simulator, thread_terminator;

    pthread_mutex_init(&print_lock, 0);
    pthread_mutex_init(&readyQ_lock, 0);
    pthread_mutex_init(&terminatedQ_lock, 0);

    // Create threads for generator, simulator and terminator
    pthread_create(&thread_generator, NULL, generator, NULL);
    pthread_create(&thread_simulator, NULL, simulator, NULL);
    pthread_create(&thread_terminator, NULL, terminator, NULL);

    // Create semaphores
    sem_init(&generator_sem, 0, MAX_CONCURRENT_PROCESSES);
    sem_init(&simulator_sem,0,0);
    sem_init(&terminator_sem, 0, 0);

    // Wait for the threads to complete
    pthread_join(thread_generator, NULL);
    pthread_join(thread_simulator, NULL);
    pthread_join(thread_terminator, NULL);

    pthread_mutex_destroy(&print_lock);
    pthread_mutex_destroy(&readyQ_lock);
    pthread_mutex_destroy(&terminatedQ_lock);

    sem_destroy(&generator_sem);
    sem_destroy(&simulator_sem);
    sem_destroy(&terminator_sem);
    
    return 0;
}
