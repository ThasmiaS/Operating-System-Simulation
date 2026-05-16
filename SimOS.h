//Thasmia Showmir
#ifndef SIMOS_H
#define SIMOS_H
#include <iostream>
#include <vector>
#include <deque>
#include <unordered_map>
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
        /**
        * @brief Create a new process
        * - takes place in the ready-queue or immediately starts using the CPU.
        * - assigns PIDs to new
        * processes starting from 1 and increments it by 1 for each new process. 
        * @pre Allocate new PID (increment from last) + do not resuse pIDs
        * @post Add process to your process table w parent = none or 0 per your design
        * @post If CPU idle -> run on CPU; else -> back of ready queue
        */
        void NewProcess();

        void SimFork();
        void SimExit();
        void SimWait();
        void TimerInterrupt();
        void DiskReadRequest(int diskNumber, std::string fileName);
        void DiskJobCompleted(int diskNumber);
        /**
        * @brief Access a memory address
        * @pre Running process only (caller must enforce CPU non-idle — see section 10).
        * @post Compute page number for this process from address and pageSize.
            * If page already in a frame → update LRU; done.
            * If not resident: if a free frame exists, pick lowest-numbered free frame; 
                * else evict LRU frame (and if that frame held another process's page, that mapping is removed).
            * Map page → frame for current PID. 
        * @param address: the address to access
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
        std::unordered_map<int, int> pageTable;
        std::deque<int> lruList;
        std::deque<int> freeFrames;
};


#endif