//Thasmia Showmir

#include "SimOS.h"

SimOS::SimOS(int numberOfDisks, int amountOfRAM, int pageSize)
: numberOfDisks(numberOfDisks), amountOfRAM(amountOfRAM), pageSize(pageSize), cpuPid(NO_PROCESS), readyQueue() {}

void SimOS::NewProcess() {
    // Allocate new PID
    static int nextPID = 0; //var created 1x and never destroyed
    nextPID++; //increment from last
    // Add process to your process table with parent = none or 0 per your design
    Process p;
    p.pid = nextPID;
    p.parentPid = 0;

    // Add to process table
    processTable[nextPID] = p;

    if (cpuPid == NO_PROCESS) { //if CPU idle -> run on CPU
        cpuPid = nextPID;
    } else { //else -> back of ready queue
        readyQueue.push_back(nextPID);
    }
}

// a process is trying to use memory
void SimOS::AccessMemoryAddress(int address) {
    if (cpuPid == NO_PROCESS) 
        throw std::logic_error("CPU is idle");
    // calculate the page number -- process wants page pageNumber
    int pageNumber = address / pageSize;

    // Page already loaded in RAM
    if (pageTable.find(pageNumber) != pageTable.end()) {
        int frame = pageTable[pageNumber];
        auto it = std::find(lruList.begin(), lruList.end(), frame);
        // if page in LRU list, remove it -- add it to front
        if (it != lruList.end())
            lruList.erase(it);
        lruList.push_front(frame);
    } 
    else { // Page fault -- page not in RAM 
        if (freeFrames.empty()) 
            throw std::logic_error("No free frames");
        // if a free frame exists, pick lowest-numbered free frame
        int freeFrame = freeFrames.front();
        freeFrames.pop_front();
        // map page to frame
        pageTable[pageNumber] = freeFrame;
        // add frame to LRU list
        lruList.push_front(freeFrame);
    }
}

void SimOS::GetMemory() {}

std::deque<int> SimOS::GetReadyQueue() {
    return readyQueue;
}

int SimOS::GetCPU() {
    return cpuPid;
}

void SimOS::GetDisk(int diskNumber) {
    (void)diskNumber;
}

void SimOS::GetDiskQueue(int diskNumber) {
    (void)diskNumber;
}