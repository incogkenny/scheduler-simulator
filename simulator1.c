#include <stdio.h>
#include <time.h>
#include "coursework.h"


int main(){
    Process *process1;
    int iPID = 1;
    long int response, turnaro;
    bool bsim = false;

    process1 = generateProcess(iPID);
    printf("GENERATOR - CREATED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",process1->iPID,process1->iPriority, process1->iBurstTime, process1->iRemainingBurstTime);
    while (process1->iState != TERMINATED){
        runPreemptiveProcess(process1, bsim);
        printf("SIMULATOR - CPU 0: RR [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n" ,process1->iPID ,process1->iPriority, process1->iBurstTime, process1->iRemainingBurstTime);
    }
    response = getDifferenceInMilliSeconds(process1->oTimeCreated, process1->oFirstTimeRunning);
    turnaro = getDifferenceInMilliSeconds(process1->oTimeCreated, process1->oLastTimeRunning);
    printf("TERMINATOR - TERMINATED: [PID = %d, ResponseTime = %ld, TurnAroundTime = %ld]\n",process1->iPID, response, turnaro);
    destroyProcess(process1);

}
