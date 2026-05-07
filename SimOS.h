//Thasmia Showmir
#ifndef SIMOS_H
#define SIMOS_H
#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>
#include <cmath>
#include <stdexcept>

constexpr int NO_PROCESS{0};
struct Process {
    int pid;
    int parentPid;
};
class SimOS {
    public:
        /** Constructor
        * @brief Initialize empty ready queue, idle CPU, empty memory book keeping, empty disk structures
        * @pre pageSize > 0, RAM multiple of page size
        * @param numberOfDisks: number of disks
        * @param amountOfRAM: total RAM size
        * @param pageSize: each memory page size
        */
        SimOS(int numberOfDisks, int amountOfRAM, int pageSize);
        /**
        * @brief Create a new process
        * @pre Allocate new PID (increment from last).
        * @post Add process to your process table w parent = none or 0 per your design
        * @post If CPU idle -> run on CPU; else -> back of ready queue
        */
        void NewProcess();
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
        void AccessMemoryAddress(int address);
        /**
         * @brief Get the memory usage
         * @return the memory usage
         */
        void GetMemory();
        /**
         * @brief Get the ready queue
         * @return the ready queue
         */
        std::deque<int> GetReadyQueue();
        int GetCPU();
        /**
         * @brief Get the disk
         * @param diskNumber: the disk number
         * @return the disk
         */
        void GetDisk(int diskNumber);
        /**
         * @brief Get the disk queue
         * @param diskNumber: the disk number
         * @return the disk queue
         */
        void GetDiskQueue(int diskNumber);

    private:
        int numberOfDisks;
        int amountOfRAM;
        int pageSize;
        int cpuPid;
        std::deque<int> readyQueue;
        std::unordered_map<int, Process> processTable;
        std::unordered_map<int, int> pageTable;
        std::deque<int> lruList;
        std::deque<int> freeFrames;
};


#endif