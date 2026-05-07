#include "SimOS.h"

int main() {
    SimOS simOS(1, 1024, 1024);
    simOS.NewProcess();
    simOS.AccessMemoryAddress(0);
    simOS.GetMemory();
    simOS.GetReadyQueue();
    simOS.GetCPU();
    simOS.GetDisk(0);
    simOS.GetDiskQueue(0);
    return 0;
}