#include <iostream>
#include "emulator_base.h"
#include "os_core.h"
#include "memory_manager.h"

int main (int argc, char**argv)
{
    if (argc != 3){
        std::cerr << "Usage: prog exeFile debugOption\n";
        exit(1);
    }
    int DEBUG = atoi(argv[2]);

    Memory mem(0x100000);
    CPU8080 theCPU(&mem);
    GTUOS	theOS;

    theCPU.ReadFileIntoMemoryAt(argv[1], 0x0000);
    for(int i=0;i<1000;i++){
        std::cout <<(int)mem.physicalAt(i);
    }

    int i=1;
    do
    {
        setbuf(stdout, 0);
        fflush(stdout);
        theCPU.Emulate8080p(DEBUG);
        if(theCPU.isSystemCall() && theCPU.interrupt==0){
            theOS.handleCall(theCPU,DEBUG);
        }
        i++;

    }	while (!theCPU.isHalted())
            ;
    return 0;
}

