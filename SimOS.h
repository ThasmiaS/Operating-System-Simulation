//Thasmia Showmir
#ifndef SIMOS_H
#define SIMOS_H
#include <iostream>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <string>
struct FileReadRequest{
    int PID{0};
    std::string fileName{""};
};
struct MemoryItem{
    unsigned long long pageNumber;
    unsigned long long frameNumber;
    int PID; // PID of the process using this frame of memory
};
using MemoryUsage = std::vector<MemoryItem>;
constexpr int NO_PROCESS{0};

struct Process {
    int pid;
    int parentPid;
};
class SimOS {
    public:
        /** Constructor
        * 
        * @brief Initialize empty ready queue, idle CPU, empty memory book keeping, empty disk structures
        * @pre pageSize > 0, RAM multiple of page size. Disks, frame, page enumerations all start from 0.
        * @param numberOfDisks: number of hard disks
        * @param amountOfRAM: total RAM size/ memory
        * @param pageSize: each memory page size
        */
        SimOS(int numberOfDisks, unsigned long long amountOfRAM, unsigned int pageSize);
        /** NewProcess
        * @brief Create a new process
        * - takes place in the ready-queue or immediately starts using the CPU.
        * - assigns PIDs to new
        * processes starting from 1 and increments it by 1 for each new process. 
        * @pre Allocate new PID (increment from last) + do not resuse pIDs
        * @post Add process to your process table w parent = none or 0 per your design
        * @post If CPU idle -> run on CPU; else -> back of ready queue
        */
        void NewProcess();
        /** SimFork
        * @brief The currently running process creates child process 
        * - Give new PID to child
        * - Add child to end of ready Q
        * - Record parent/child relationship.
        * @pre Running process 
        * @post new child process exists & is placed at end of ready Q
        */
        void SimFork();
        /** SimExit
        * @brief process that is currently using CPU terminates. 
        * release the memory immediately. 
        * parent is already waiting --> process terminates immediately + parent becomes runnable (go to ready Q) 
        * parent hasn't called wait --> process turns zombie
        * system implements the cascading termination to avoid appearance of orphans
        * - Cascading termination: process terminates --> all its descendants terminate w it
        */
        void SimExit();
        /** SimWait
        process wants to pause + wait for any of its child processes to terminate. 
        Once wait is over --> process goes to end of  ready Q or CPU. 
        If zombie child already exists --> process proceeds right away (keeps using CPU) 
            + the zombie-child disappears
        
            If 1+ zombie-child exists --> system uses 1 of them (any!) to immediately resume parent
            + other zombies keep waiting for the next wait from parent + parent resumes immediately
        */
        void SimWait();
        /** TimerInterrupt
        Interrupt arrives from timer signaling time slice of curr runningprocess is over
        process moves to end of ready Q
        */
        void TimerInterrupt();
        /** DiskReadRequest
        Curr running process requests to read specified file from disk w a given number
        The process issuing disk reading requests immediately stops using CPU
            - even if ready Q is empty
        */
        void DiskReadRequest(int diskNumber, std::string fileName);
        /** DiskJobCompleted
        disk w a specified # reports that a 1 job is completed
        served process return to ready Q
        */
        void DiskJobCompleted(int diskNumber);
        /** access memory address
        Curre running process wants to access specified logical memory address. 
        System makes sure corresponding page is loaded in RAM. 
        If the corresponding page is already in the RAM, its “recently used” info is updated. 
        When multiple frames are available, system chooses the one w the lower number        
        */
        void AccessMemoryAddress(unsigned long long address);
        /**
         * @brief Get the memory usage
         * @return the memory usage
         */
        int GetCPU();
        /**
         * @brief Get the ready queue
         * @return the ready queue
         */
        std::deque<int> GetReadyQueue();
        MemoryUsage GetMemory();
        FileReadRequest GetDisk(int diskNumber);
        std::deque<FileReadRequest> GetDiskQueue(int diskNumber);


    private:
        int numberOfDisks;
        int amountOfRAM;
        int pageSize;
        int nextPid;
        int cpuPid;
        std::deque<int> readyQueue;
        std::unordered_map<int, Process> processTable;
        std::vector<MemoryItem> memoryUsage;
        
        std::deque<int> lruList;
        std::deque<int> freeFrames;
        std::unordered_set<int> waitingProcesses;
        std::unordered_map<int, std::deque<int>> zombieChildren;
        std::vector<std::deque<FileReadRequest>> diskQueues;

        bool hasZombieChild(int parentPid) const;
        void removeOneZombieChild(int parentPid);
};


#endif