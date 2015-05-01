#include "VirtualMachine.h"
#include "Machine.h"

#include "unistd.h"
#include "errno.h"

#include "iostream" //delete this
using namespace std; //also this

extern "C"
{
  //Implemented in VirtualMachineUtils.c
  TVMMainEntry VMLoadModule(const char *module);
  void VMUnloadModule(void);
  TVMStatus VMFilePrint(int filedescriptor, const char *format, ...);
  
  
  volatile int ticks = 0;
  
  void AlarmCallback (void *calldata)
  {
    ticks--;
  }
 
  TVMStatus VMStart(int tickms, int machinetickms, int argc, char *argv[])
  {
    const char *module = argv[0];
    if(machinetickms <= 0)
    {
      machinetickms = 10;
    }
    if(tickms <= 0)
    {
      tickms = 10;
    }
    
    TVMMainEntry VMMain = VMLoadModule(module);
    if (VMMain == NULL)
    {
      return VM_STATUS_FAILURE;
    }

    MachineInitialize(machinetickms);
    MachineRequestAlarm(tickms * 1000, AlarmCallback, NULL);
    MachineEnableSignals();
    VMMain(argc, argv);
    return VM_STATUS_SUCCESS;
  }

  TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid);
  TVMStatus VMThreadDelete(TVMThreadID thread);
  TVMStatus VMThreadActivate(TVMThreadID thread);
  TVMStatus VMThreadTerminate(TVMThreadID thread);
  TVMStatus VMThreadID(TVMThreadIDRef threadref);
  TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref);
  
  TVMStatus VMThreadSleep(TVMTick tick)
  {
    if(tick == VM_TIMEOUT_INFINITE)
    {
      return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    ticks = tick;
    while (ticks > 0)
    {
      
    }
    return VM_STATUS_SUCCESS;
  }

  TVMStatus VMMutexCreate(TVMMutexIDRef mutexref);
  TVMStatus VMMutexDelete(TVMMutexID mutex);
  TVMStatus VMMutexQuery(TVMMutexID mutex, TVMThreadIDRef ownerref);
  TVMStatus VMMutexAcquire(TVMMutexID mutex, TVMTick timeout);     
  TVMStatus VMMutexRelease(TVMMutexID mutex);
  
  TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor);
  TVMStatus VMFileClose(int filedescriptor);      
  TVMStatus VMFileRead(int filedescriptor, void *data, int *length);
  
  //TODO convert to using MachineFileWrite
  TVMStatus VMFileWrite(int filedescriptor, void *data, int *length)
  {
    if (data == NULL || length == NULL)
    {
      return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    write(filedescriptor, data, *length);
    // if(success)
    // {
      return VM_STATUS_SUCCESS;
    // }
    // else
    // {
      // return VM_STATUS_FAILURE;
    // }
  }
}