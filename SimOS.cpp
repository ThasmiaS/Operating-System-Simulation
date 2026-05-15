//Thasmia Showmir

#include "SimOS.h"

SimOS::SimOS(int numberOfDisks, unsigned long long amountOfRAM, unsigned int pageSize){

    if (pageSize <= 0 || amountOfRAM % pageSize != 0){  
        throw std::invalid_argument("Invalid page size or amount of RAM");
    }

    this->numberOfDisks = numberOfDisks;
    this->amountOfRAM = amountOfRAM;
    this->pageSize = pageSize;

    this->readyQueue.clear();
    this->cpuPid = NO_PROCESS;
    pageTable.clear();
    freeFrames.clear();

    const int frameCount = amountOfRAM / pageSize;
    for (int f = 0; f < frameCount; ++f) {
        freeFrames.push_back(f);
    }
}