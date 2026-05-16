//Thasmia Showmir

#include "SimOS.h"

SimOS::SimOS(int numberOfDisks, unsigned long long amountOfRAM, unsigned int pageSize){
    this->nextPid = 1;
    if (pageSize == 0 || amountOfRAM % pageSize != 0){  
        throw std::invalid_argument("Invalid page size or amount of RAM");
    }

    this->numberOfDisks = numberOfDisks;
    this->amountOfRAM = amountOfRAM;
    this->pageSize = pageSize;

    this->readyQueue.clear(); // no processes waiting to run
    this->cpuPid = NO_PROCESS;
    pageTable.clear();
    freeFrames.clear();

    const int frameCount = amountOfRAM / pageSize;
    for (int f = 0; f < frameCount; ++f) { //every frame is initially available
        //fill frame list w [0, 1, 2, 3, ..., frameCount-1]
        freeFrames.push_back(f);
    }
}

void SimOS::NewProcess () {
    const int pid = nextPid++;

    Process newProcess{pid, 0};
    processTable[pid] = newProcess;

    if (cpuPid == NO_PROCESS) 
        cpuPid = pid; // run immediately on CPU
    else 
        readyQueue.push_back(pid);//add to ready queue / get in line
}






