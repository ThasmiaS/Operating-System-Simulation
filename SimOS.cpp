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
void SimOS::TerminateProcessTree(int pid) {

    // 1. recursively terminate children first
    std::vector<int> children;

    for (const auto &entry : processTable) {
        if (entry.second.parentPid == pid) {
            children.push_back(entry.first);
        }
    }

    for (int childPid : children) {
        TerminateProcessTree(childPid);
    }

    // 2. free memory
    for (auto it = memoryUsage.begin();
         it != memoryUsage.end();) {

        if (it->PID == pid) {

            freeFrames.push_back(it->frameNumber);

            lruList.erase(
                std::remove(lruList.begin(),
                            lruList.end(),
                            it->frameNumber),
                lruList.end());

            it = memoryUsage.erase(it);
        }
        else {
            ++it;
        }
    }

    // 3. remove from ready queue
    readyQueue.erase(
        std::remove(readyQueue.begin(),
                    readyQueue.end(),
                    pid),
        readyQueue.end());

    // 4. remove from waiting set
    waitingProcesses.erase(pid);

    // 5. remove from disk queues
    for (auto &queue : diskQueues) {

        queue.erase(
            std::remove_if(queue.begin(),
                           queue.end(),
                           [pid](const FileReadRequest &r) {
                               return r.PID == pid;
                           }),
            queue.end());
    }

    // 6. remove zombie records
    zombieChildren.erase(pid);

    for (auto &[parent, zombies] : zombieChildren) {

        zombies.erase(
            std::remove(zombies.begin(),
                        zombies.end(),
                        pid),
            zombies.end());
    }

    // 8. remove process itself
    processTable.erase(pid);
}
void SimOS::SimExit() {
    if (cpuPid == NO_PROCESS)
        throw std::logic_error("No running process");

    int pid = cpuPid;

    // -----------------------------
    // Cascading termination
    // -----------------------------
    std::vector<int> children;

    for (const auto &entry : processTable) {
        if (entry.second.parentPid == pid) {
            children.push_back(entry.first);
        }
    }

    for (int childPid : children) {
        TerminateProcessTree(childPid);
    }

    // -----------------------------
    // Free memory frames
    // -----------------------------
    for (auto it = memoryUsage.begin();it != memoryUsage.end();) {

        if (it->PID == pid) {
            // free frame
            freeFrames.push_back(it->frameNumber);

            // remove from LRU
            lruList.erase(std::remove(lruList.begin(), lruList.end(), it->frameNumber), lruList.end());

            // erase memory entry
            it = memoryUsage.erase(it);
        }
        else ++it;
    }

    int parentPid = processTable[pid].parentPid;

    // -----------------------------
    // Parent already waiting
    // -----------------------------
    if (waitingProcesses.count(parentPid)) {
        waitingProcesses.erase(parentPid);

        // parent becomes runnable
        readyQueue.push_back(parentPid);

        // child fully removed
        processTable.erase(pid);
    }
    else if (parentPid == 0) {

        // no parent -> fully remove process
        processTable.erase(pid);
    }
    else {
    
        // process becomes zombie
        zombieChildren[parentPid].push_back(pid);
    }
    

    // -----------------------------
    // Schedule next process
    // -----------------------------
    if (!readyQueue.empty()) {
        cpuPid = readyQueue.front();
        readyQueue.pop_front();
    }
    else cpuPid = NO_PROCESS;
    
}

void SimOS::SimWait() {
    if (cpuPid == NO_PROCESS)
        throw std::logic_error("No running process");

    int pid = cpuPid;

    auto zombieIt = zombieChildren.find(pid);

    if (zombieIt != zombieChildren.end() &&
        !zombieIt->second.empty()) {

        // reap any zombie child
        int childPid = *zombieIt->second.begin();

        zombieIt->second.erase(std::remove(zombieIt->second.begin(), zombieIt->second.end(), childPid), zombieIt->second.end());

        // remove zombie list entry if empty
        if (zombieIt->second.empty()) {
            zombieChildren.erase(zombieIt);
        }

        // fully remove zombie process
        processTable.erase(childPid);
        // parent keeps CPU
        return;
    }

    // No zombie child:
    // process blocks waiting
    waitingProcesses.insert(pid);

    // Remove process from CPU
    cpuPid = NO_PROCESS;

    // Schedule next ready process
    if (!readyQueue.empty()) {
        cpuPid = readyQueue.front();
        readyQueue.pop_front();
    }
    else cpuPid = NO_PROCESS;
    
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
    if (diskNumber < 0 || diskNumber >= numberOfDisks)
        throw std::out_of_range("Invalid disk number");

    if (diskQueues[diskNumber].empty())
        throw std::logic_error("No disk job in progress");

    const int completedPid = diskQueues[diskNumber].front().PID;
    diskQueues[diskNumber].pop_front();

    if (cpuPid == NO_PROCESS)
        cpuPid = completedPid;
    else
        readyQueue.push_back(completedPid);
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

int SimOS::GetCPU() {
    return cpuPid;
}

std::deque<int> SimOS::GetReadyQueue() {
    return readyQueue;
}

MemoryUsage SimOS::GetMemory() {

    MemoryUsage result = memoryUsage;

    std::sort(result.begin(), result.end(),
        [](const MemoryItem &a, const MemoryItem &b) {
            return a.frameNumber < b.frameNumber;
        });

    return result;
}

FileReadRequest SimOS::GetDisk(int diskNumber) {

    if (diskNumber < 0 || diskNumber >= numberOfDisks)
        throw std::out_of_range("Invalid disk number");

    // Disk idle
    if (diskQueues[diskNumber].empty())
        return FileReadRequest{0, ""};

    // Current job = front
    return diskQueues[diskNumber].front();
}


std::deque<FileReadRequest>
SimOS::GetDiskQueue(int diskNumber) {

    if (diskNumber < 0 || diskNumber >= numberOfDisks)
        throw std::out_of_range("Invalid disk number");

    std::deque<FileReadRequest> waitingQueue;

    auto &queue = diskQueues[diskNumber];

    // Skip current job at front
    for (size_t i = 1; i < queue.size(); ++i) {
        waitingQueue.push_back(queue[i]);
    }

    return waitingQueue;
}