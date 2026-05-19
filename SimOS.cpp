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

void SimOS::SimFork() {
    if (cpuPid == NO_PROCESS) 
        throw std::logic_error("No running process");
    
    const int pid = nextPid++;
    Process newProcess{pid, cpuPid};
    processTable[pid] = newProcess;
    readyQueue.push_back(pid);
}

void SimOS::SimExit() {
    if (cpuPid == NO_PROCESS) 
        throw std::logic_error("No running process");
    
    const int pid = cpuPid;
    const int parentPid = processTable[pid].parentPid;
    if (parentPid != 0) 
        readyQueue.push_back(parentPid); // parent becomes runnable
    processTable.erase(pid); // remove process from table
    pageTable.erase(pid); // remove page table entries
    freeFrames.push_back(pageTable[pid]); // add free frames to free frames list
    cpuPid = NO_PROCESS; // CPU is idle
}

