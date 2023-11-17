#include <stdio.h>
#include "coursework.h"
#include "linkedlist.h"

int main(){
    int process_count;
    int readyQ_length = 0;
    int terminatedQ_length = 0;
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
    printf("GENERATOR: Finished");

    while(readyQ_length != 0){
        

    }




}
