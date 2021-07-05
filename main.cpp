#include <iostream>
#include "advanced_simulator.hpp"
//#include "simulator.hpp"
#include "memory.hpp"

memoryManager mManager;
Simulator simulator;

int main()
{
//    freopen("../testcases/bulgarian.data", "r", stdin);
//    freopen("my_reg", "w", stdout);
//    freopen("pipeline", "w", stdout);
    mManager.init();
    std::cout << simulator.run() << std::endl;

#ifdef print
    simulator.printReg();
#endif
    return 0;
}

