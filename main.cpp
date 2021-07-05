#include <iostream>
#include "five_stage_pipeline_simulator.hpp"
#include "memory.hpp"

memoryManager mManager;
Simulator simulator;

int main()
{
//    freopen("../sample/sample.data", "r", stdin);
//    freopen("../testcases/tak.data", "r", stdin);
    mManager.init();
    std::cout << simulator.run() << std::endl;
//    simulator.printReg();
    return 0;
}

