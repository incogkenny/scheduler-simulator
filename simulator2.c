#include <stdio.h>
#include <stdlib.h>
#include "coursework.h"
#include "linkedlist.h"

int main(){
    int process_count;
    int terminated_count = 0;
    int readyQ_length = 0;
    int terminatedQ_length = 0;
    long int total_response, total_turnaround;
    double mean_r, mean_tu;
    bool bsim = false;
    Process *current_process;
    LinkedList oReadyQueue = LINKED_LIST_INITIALIZER;
    LinkedList oTerminatedQueue = LINKED_LIST_INITIALIZER;


    // Initialise processes into process queue
    for (process_count=0; process_count< NUMBER_OF_PROCESSES; process_count++){
        
        // Creates and prints current process
        current_process = generateProcess(process_count);
        printf("GENERATOR - CREATED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",current_process->iPID,current_process->iPriority, current_process->iBurstTime, current_process->iRemainingBurstTime);

        // Adds process to oReadyQueue and prints that process was added to ready queue and that process is admitted 
        addLast(current_process, &oReadyQueue);
        readyQ_length++;
        printf("QUEUE - ADDED: [Queue = READY, Size = %d, PID = %d, Priority = %d]\n", readyQ_length, current_process->iPID, current_process->iPriority);
        printf("GENERATOR - ADMITTED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n", current_process->iPID,current_process->iPriority, current_process->iBurstTime, current_process->iRemainingBurstTime);
    }
    printf("GENERATOR: Finished\n");

    // Simulates processes being run 
    while(readyQ_length != 0){
        current_process = getHead(oReadyQueue)->pData;
        removeFirst(&oReadyQueue);
        readyQ_length--;
        printf("QUEUE - REMOVED: [Queue = READY, Size = %d, PID = %d, Priority = %d]\n", readyQ_length, current_process->iPID, current_process->iPriority);
        runPreemptiveProcess(current_process, bsim);
        printf("SIMULATOR - CPU 0: RR [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n", current_process->iPID, current_process->iPriority, current_process->iBurstTime, current_process->iRemainingBurstTime);

        if(current_process->iState == READY){
            addLast(current_process, &oReadyQueue);
            readyQ_length++;
            printf("QUEUE - ADDED: [Queue = READY, Size = %d, PID = %d, Priority = %d]\n", readyQ_length, current_process->iPID, current_process->iPriority);
            printf("SIMULATOR - CPU 0 - READY: [PID = %d, Priority = %d]\n", current_process->iPID, current_process->iPriority);

        }

        if(current_process->iState == TERMINATED){
            printf("SIMULATOR - CPU 0 - TERMINATED: [PID = %d, ResponseTime = %ld, TurnAroundTime = %ld]\n", current_process->iPID, getDifferenceInMilliSeconds(current_process->oTimeCreated, current_process->oFirstTimeRunning), getDifferenceInMilliSeconds(current_process->oTimeCreated, current_process->oLastTimeRunning));
            addLast(current_process, &oTerminatedQueue);
            terminatedQ_length++;
            printf("QUEUE - ADDED: [Queue = TERMINATED, Size = %d, PID = %d, Priority = %d]\n", terminatedQ_length, current_process->iPID, current_process->iPriority);
        }
    }

    printf("SIMULATOR: Finished\n");

    while(terminated_count != 10){
        current_process = removeFirst(&oTerminatedQueue);
        terminatedQ_length--;
        terminated_count++;
        printf("QUEUE - REMOVED: [Queue = TERMINATED, Size = %d, PID = %d, Priority = %d]\n", terminatedQ_length, current_process->iPID, current_process->iPriority);
        total_response += getDifferenceInMilliSeconds(current_process->oTimeCreated, current_process->oFirstTimeRunning);
        total_turnaround += getDifferenceInMilliSeconds(current_process->oTimeCreated, current_process->oLastTimeRunning);
        printf("TERMINATION DAEMON - CLEARED: [#iTerminated = %d, PID = %d, Priority = %d]\n", terminated_count, current_process->iPID, current_process->iPriority);
        destroyProcess(current_process);
    }
    mean_r = (double)total_response;
    mean_tu = (double)total_turnaround;
    printf("TERMINATION DAEMON: Finished\n");
    printf("TERMINATION DAEMON: [Average Response Time = %lf, Average Turn Around Time = %lf]\n", (mean_r/NUMBER_OF_PROCESSES), (mean_tu/NUMBER_OF_PROCESSES));
    
}
