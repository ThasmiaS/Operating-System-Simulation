//Thasmia Showmir

#include "SimOS.h"
#include <algorithm>
#include <stdexcept>

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
    memoryUsage.clear();
    lruList.clear();
    freeFrames.clear();
    waitingProcesses.clear();
    zombieChildren.clear();
    diskQueues.resize(numberOfDisks);

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

    // Cannot terminate if no process is running
    if (cpuPid == NO_PROCESS) 
        throw std::logic_error("No running process");
    
    // PID of process currently using CPU
    const int pid = cpuPid;
    
    // Find all frames owned by this process
    auto it = std::find_if(memoryUsage.begin(), memoryUsage.end(), [pid](const MemoryItem &item) { return item.PID == pid; });

    if (it != memoryUsage.end()) { // if process has frames

        // Free every frame used by process
        for (const auto &item : memoryUsage) {

            // Return frame to free frame list
            freeFrames.push_back(item.frameNumber);

            // Remove frame from LRU tracking list
            auto lruIt = std::find(lruList.begin(), lruList.end(), item.frameNumber);

            if (lruIt != lruList.end())
                lruList.erase(lruIt);
            
        }

        // Remove process - frame mapping
        memoryUsage.erase(it);
    }

    // Remove process from process table
    processTable.erase(pid);

    // Schedule next ready process
    if (!readyQueue.empty()) {
        cpuPid = readyQueue.front();
        readyQueue.pop_front();
    }
    else 
        // No runnable processes left
        cpuPid = NO_PROCESS;
}

void SimOS::SimWait() {

    // Cannot wait if no process is running
    if (cpuPid == NO_PROCESS)
        throw std::logic_error("No running process");

    int pid = cpuPid;

    // Look for a zombie child
    for (auto it = zombieChildren.begin(); it != zombieChildren.end(); ++it) {

        const int childPid = it->first;

        // Check if zombie belongs to current process
        if (processTable[childPid].parentPid == pid) {

            // Remove zombie child immediately
            processTable.erase(childPid);
            zombieChildren.erase(it); // remove zombie child from zombie children list

            // Parent keeps CPU
            return;
        }
    }

    // No zombie child exists:
    // current process becomes waiting
    waitingProcesses.insert(pid);

    // Schedule next ready process
    if (!readyQueue.empty()) {

        cpuPid = readyQueue.front();
        readyQueue.pop_front();
    }
    else 
        cpuPid = NO_PROCESS;
}

void SimOS::TimerInterrupt() {

    // Timer interrupt requires a running process
    if (cpuPid == NO_PROCESS)
        throw std::logic_error("No running process");
    
    // Move current process to back of ready queue
    readyQueue.push_back(cpuPid);

    // Run next ready process
    cpuPid = readyQueue.front();
    readyQueue.pop_front();
}

void SimOS::DiskReadRequest(int diskNumber, std::string fileName) {
    if (cpuPid == NO_PROCESS)
        throw std::logic_error("No running process");

    // Invalid disk number
    if (diskNumber < 0 || diskNumber >= numberOfDisks)
        throw std::out_of_range("Invalid disk number");

    // Create disk read request
    FileReadRequest request{cpuPid, fileName};

    // Add request to selected disk queue
    diskQueues[diskNumber].push_back(request);

    // Process blocks waiting for disk
    cpuPid = NO_PROCESS;

    // Schedule next ready process
    if (!readyQueue.empty()) {
        cpuPid = readyQueue.front();
        readyQueue.pop_front();
    }
}

void SimOS::DiskJobCompleted(int diskNumber) {
    // Invalid disk number
    if (diskNumber < 0 || diskNumber >= numberOfDisks)
        throw std::out_of_range("Invalid disk number");
    
    // No active disk job
    if (diskQueues[diskNumber].empty())
        throw std::logic_error("No disk job in progress");
    
    // Get completed request
    const FileReadRequest request = diskQueues[diskNumber].front();

    // Remove completed request from disk queue
    diskQueues[diskNumber].pop_front();

    // If CPU idle, process runs immediately
    if (cpuPid == NO_PROCESS) 
        cpuPid = request.PID;
    else
        // Otherwise process goes to ready queue
        readyQueue.push_back(request.PID);

}

void SimOS::AccessMemoryAddress(unsigned long long address) {

    if (cpuPid == NO_PROCESS)
        throw std::logic_error("No running process");

    const unsigned long long pageNumber = address / pageSize;

    // 1. Check if page already in memory
    for (auto &item : memoryUsage) {

        if (item.PID == cpuPid && item.pageNumber == pageNumber) {

            // update LRU
            auto it = std::find(lruList.begin(), lruList.end(), item.frameNumber);
            if (it != lruList.end())
                lruList.erase(it);

            lruList.push_back(item.frameNumber);
            return;
        }
    }

    // 2. Need a frame
    int frame;

    if (freeFrames.empty()) {

        // eviction
        int victimFrame = lruList.front();
        lruList.pop_front();

        // remove old mapping
        for (auto it = memoryUsage.begin(); it != memoryUsage.end(); ++it) {
            if (it->frameNumber == victimFrame) {
                memoryUsage.erase(it);
                break;
            }
        }

        frame = victimFrame; // reuse freed frame
    }
    else {

        auto minIt = std::min_element(freeFrames.begin(), freeFrames.end());
        frame = *minIt;
        freeFrames.erase(minIt);
    }

    // 3. load page
    memoryUsage.push_back({pageNumber, (unsigned long long)frame, cpuPid});
    lruList.push_back(frame);
}