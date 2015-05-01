#include "VirtualMachine.h"

extern "C"
{
  //Implemented in VirtualMachineUtils.c
  TVMMainEntry VMLoadModule(const char *module);
  void VMUnloadModule(void);
  TVMStatus VMFilePrint(int filedescriptor, const char *format, ...);
  
  //code goes here, implement these functions
  TVMStatus VMStart(int tickms, int machinetickms, int argc, char *argv[]);

  TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid);
  TVMStatus VMThreadDelete(TVMThreadID thread);
  TVMStatus VMThreadActivate(TVMThreadID thread);
  TVMStatus VMThreadTerminate(TVMThreadID thread);
  TVMStatus VMThreadID(TVMThreadIDRef threadref);
  TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref);
  TVMStatus VMThreadSleep(TVMTick tick);

  TVMStatus VMMutexCreate(TVMMutexIDRef mutexref);
  TVMStatus VMMutexDelete(TVMMutexID mutex);
  TVMStatus VMMutexQuery(TVMMutexID mutex, TVMThreadIDRef ownerref);
  TVMStatus VMMutexAcquire(TVMMutexID mutex, TVMTick timeout);     
  TVMStatus VMMutexRelease(TVMMutexID mutex);
  
  TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor);
  TVMStatus VMFileClose(int filedescriptor);      
  TVMStatus VMFileRead(int filedescriptor, void *data, int *length);
  TVMStatus VMFileWrite(int filedescriptor, void *data, int *length);
}