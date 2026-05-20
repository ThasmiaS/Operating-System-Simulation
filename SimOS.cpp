//Name: Thasmia Showmir

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

    this->readyQueue.clear(); // ready q empty
    this->cpuPid = NO_PROCESS;
    memoryUsage.clear();
    lruList.clear();
    freeFrames.clear();
    waitingProcesses.clear();
    zombieChildren.clear();
    diskQueues.resize(numberOfDisks);

    const int frameCount = amountOfRAM / pageSize;
    for (int f = 0; f < frameCount; ++f) { // all frames start free
        // list frames 0,1,2... up to frameCount-1
        freeFrames.push_back(f);
    }
}

void SimOS::NewProcess () {
    const int pid = nextPid++;

    Process newProcess{pid, 0};
    processTable[pid] = newProcess;

    if (cpuPid == NO_PROCESS) 
        cpuPid = pid; // cpu was idle so run now
    else 
        readyQueue.push_back(pid);// back of ready Q
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

    // kill child procs first (recursive)
    std::vector<int> children;

    for (const auto &entry : processTable) {
        if (entry.second.parentPid == pid) {
            children.push_back(entry.first);
        }
    }

    for (int childPid : children) {
        TerminateProcessTree(childPid);
    }

    // free up its memory frames
    for (auto it = memoryUsage.begin();it != memoryUsage.end();) {

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

    // take pid out of ready Q
    readyQueue.erase(
        std::remove(readyQueue.begin(),
                    readyQueue.end(),
                    pid),
        readyQueue.end());

    // not blocked on wait anymore
    waitingProcesses.erase(pid);

    // remove from all disk queues
    for (auto &queue : diskQueues) {

        queue.erase(
            std::remove_if(queue.begin(),queue.end(),
                [pid](const FileReadRequest &r) {
                    return r.PID == pid;
            }),
            queue.end());
    }

    // clean zombie lists
    zombieChildren.erase(pid);

    for (auto &[parent, zombies] : zombieChildren) {

        zombies.erase(
            std::remove(zombies.begin(),
                        zombies.end(),
                        pid),
            zombies.end());
    }

    // finally erase from process table
    processTable.erase(pid);
}
void SimOS::SimExit() {
    if (cpuPid == NO_PROCESS)
        throw std::logic_error("No running process");

    int pid = cpuPid;

    // cascade kill whole subtree
    std::vector<int> children;

    for (const auto &entry : processTable) {
        if (entry.second.parentPid == pid) {
            children.push_back(entry.first);
        }
    }

    for (int childPid : children) {
        TerminateProcessTree(childPid);
    }

    // free this procs frames
    for (auto it = memoryUsage.begin();it != memoryUsage.end();) {

        if (it->PID == pid) {
            // put frame back in free list
            freeFrames.push_back(it->frameNumber);

            // drop from lru tracking
            lruList.erase(std::remove(lruList.begin(), lruList.end(), it->frameNumber), lruList.end());

            // remove from memoryUsage
            it = memoryUsage.erase(it);
        }
        else ++it;
    }

    int parentPid = processTable[pid].parentPid;

    // parent was in SimWait already
    if (waitingProcesses.count(parentPid)) {
        waitingProcesses.erase(parentPid);

        // wake parent end of ready Q
        readyQueue.push_back(parentPid);

        // child gone for good
        processTable.erase(pid);
    }
    else if (parentPid == 0) {

        // root proc just delete it
        processTable.erase(pid);
    }
    else {
    
        // parent didnt wait yet -> zombie
        zombieChildren[parentPid].push_back(pid);
    }
    

    // pick who runs next on cpu
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

        // got a zombie kid reap one
        int childPid = *zombieIt->second.begin();

        zombieIt->second.erase(std::remove(zombieIt->second.begin(), zombieIt->second.end(), childPid), zombieIt->second.end());

        // empty zombie list? remove key
        if (zombieIt->second.empty()) {
            zombieChildren.erase(zombieIt);
        }

        // zombie fully gone now
        processTable.erase(childPid);
        // parent stays on cpu
        return;
    }

    // no zombie yet block
    waitingProcesses.insert(pid);

    // give up cpu
    cpuPid = NO_PROCESS;

    // run somebody else
    if (!readyQueue.empty()) {
        cpuPid = readyQueue.front();
        readyQueue.pop_front();
    }
    else cpuPid = NO_PROCESS;
    
}

void SimOS::TimerInterrupt() {

    // need someone on cpu for timer
    if (cpuPid == NO_PROCESS)
        throw std::logic_error("No running process");
    
    // time slice over ->back of ready Q
    readyQueue.push_back(cpuPid);

    // next in line gets cpu
    cpuPid = readyQueue.front();
    readyQueue.pop_front();
}

void SimOS::DiskReadRequest(int diskNumber, std::string fileName) {
    if (cpuPid == NO_PROCESS)
        throw std::logic_error("No running process");

    // bad disk #
    if (diskNumber < 0 || diskNumber >= numberOfDisks)
        throw std::out_of_range("Invalid disk number");

    // make the read request
    FileReadRequest request{cpuPid, fileName};

    // enqueue on that disk (fifo)
    diskQueues[diskNumber].push_back(request);

    // proc waits on disk not cpu
    cpuPid = NO_PROCESS;

    // dispatch next ready if any
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

    // page already loaded?
    for (auto &item : memoryUsage) {

        if (item.PID == cpuPid && item.pageNumber == pageNumber) {

            // bump lru --> was a hit
            auto it = std::find(lruList.begin(), lruList.end(), item.frameNumber);
            if (it != lruList.end())
                lruList.erase(it);

            lruList.push_back(item.frameNumber);
            return;
        }
    }

    // need to load page into a frame
    int frame;

    if (freeFrames.empty()) {

        // ram full -> kick lru frame
        int victimFrame = lruList.front();
        lruList.pop_front();

        // clear old page in that frame
        for (auto it = memoryUsage.begin(); it != memoryUsage.end(); ++it) {
            if (it->frameNumber == victimFrame) {
                memoryUsage.erase(it);
                break;
            }
        }

        frame = victimFrame; // use same frame #
    }
    else {

        auto minIt = std::min_element(freeFrames.begin(), freeFrames.end());
        frame = *minIt;
        freeFrames.erase(minIt);
    }

    // map page into frame
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

    // disk not doing anything
    if (diskQueues[diskNumber].empty())
        return FileReadRequest{0, ""};

    // whats running now is at front
    return diskQueues[diskNumber].front();
}


std::deque<FileReadRequest>
SimOS::GetDiskQueue(int diskNumber) {

    if (diskNumber < 0 || diskNumber >= numberOfDisks)
        throw std::out_of_range("Invalid disk number");

    std::deque<FileReadRequest> waitingQueue;

    auto &queue = diskQueues[diskNumber];

    // waiting line = everything after front
    for (size_t i = 1; i < queue.size(); ++i) {
        waitingQueue.push_back(queue[i]);
    }

    return waitingQueue;
}