#include "VirtualMachine.h"
#include "Machine.h"
#include "vector"

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
  
  
  struct TCB
  {
    TVMThreadID t_id;
    TVMThreadPriority t_prio;
    TVMThreadState t_state;
    TVMMemorySize t_memsize;
    TVMThreadEntry t_entry;
    uint8_t *stk_ptr;
    void *param;
    SMachineContext t_context;
    TVMTick t_ticks;
    // Possibly need something to hold file return type
    // Possibly neew mutex id.
    // Possibly hold a list of held mutexes
  }; // struct
  
  int curID;
  vector<TCB> allThreads;
  vector<TCB> readyHigh;
  vector<TCB> readyMed;
  vector<TCB> readyLow;
  vector<TCB> sleeping;
  
  void setReady(struct TCB thread)
  {
    TVMThreadPriority prio = thread.t_prio;
    switch (prio)
    {
      case VM_THREAD_PRIORITY_HIGH:
        readyHigh.push_back(thread);
        break;
      case VM_THREAD_PRIORITY_NORMAL:
        readyMed.push_back(thread);
        break;
      case VM_THREAD_PRIORITY_LOW:
        readyLow.push_back(thread);
        break;
    }
  }
  
  void scheduler()
  {
    TVMThreadID tid;
    if(!readyHigh.empty())
    {
      tid = readyHigh[0].t_id;
      readyHigh.erase(readyHigh.begin());
    }
    else if (!readyMed.empty())
    {
      tid = readyMed[0].t_id;
      readyMed.erase(readyMed.begin());
    }
    else if (!readyLow.empty())
    {
      tid = readyLow[0].t_id;
      readyLow.erase(readyLow.begin());
    }
    else
    {
      tid = 1; 
    }
    cout << "cur: " <<curID<<" new: "<<tid <<'\n';
    SMachineContext oldctx = allThreads[curID].t_context;
    SMachineContext newctx = allThreads[(int)tid].t_context;
    curID = tid;
    MachineContextSwitch(&oldctx, &newctx);
  }
  
  void AlarmCallback (void *calldata)
  {
    for(unsigned int i = 0; i < sleeping.size(); i++)
    {
      cout << "tick " << sleeping[i].t_ticks << '\n';
      sleeping[i].t_ticks--;
      if(sleeping[i].t_ticks == 0)
      {
        sleeping[i].t_state = VM_THREAD_STATE_READY;
        setReady(sleeping[i]);
        sleeping.erase(sleeping.begin()+i);
      }
      scheduler();
    }
  }
  
  void idleFunc(void *)
  {
    while (1);
  }
 
  TVMStatus VMStart(int tickms, int machinetickms, int argc, char *argv[])
  {
    curID = 0;
    const char *module = argv[0];
    if(machinetickms <= 0)
    {
      machinetickms = 10;
    }
    if(tickms <= 0)
    {
      tickms = 10;
    }
    struct TCB mainThread;
    mainThread.t_id = 0;
    mainThread.t_prio = VM_THREAD_PRIORITY_NORMAL;
    mainThread.t_state = VM_THREAD_STATE_READY;
    allThreads.push_back(mainThread);
    
    TVMThreadID idleID;
    VMThreadCreate(idleFunc, NULL, 0x100000, 0, &idleID);
    VMThreadActivate(idleID);
    
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

  TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid)
  {
    if(entry == NULL || tid == NULL)
    {
      return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    struct TCB newThread;
    TMachineSignalStateRef sigstate;
    MachineSuspendSignals(sigstate);
    newThread.t_state = VM_THREAD_STATE_DEAD;
    newThread.t_entry = entry;
    newThread.param = param;
    newThread.t_memsize = memsize;
    newThread.stk_ptr = new uint8_t[memsize];
    newThread.t_prio = prio;
    *tid = allThreads.size();
    newThread.t_id = *tid;
    allThreads.push_back(newThread);
    MachineResumeSignals(sigstate);
    return VM_STATUS_SUCCESS;
  }
  
  TVMStatus VMThreadDelete(TVMThreadID thread);
  
  TVMStatus VMThreadActivate(TVMThreadID thread)
  {
    struct TCB newThread = allThreads[(int)thread];
    TMachineSignalStateRef sigstate;
    MachineSuspendSignals(sigstate);
    MachineContextCreate(&newThread.t_context, newThread.t_entry, newThread.param, newThread.stk_ptr, newThread.t_memsize);
    newThread.t_state = VM_THREAD_STATE_READY;
    if(newThread.t_state > allThreads[curID].t_state)
    {
      setReady(newThread);
      scheduler();
    }
    MachineResumeSignals(sigstate);
    return VM_STATUS_SUCCESS;
  }
  
  TVMStatus VMThreadTerminate(TVMThreadID thread);
  TVMStatus VMThreadID(TVMThreadIDRef threadref);
  TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref);
  
  TVMStatus VMThreadSleep(TVMTick tick)
  {
    if(tick == VM_TIMEOUT_INFINITE)
    {
      return VM_STATUS_ERROR_INVALID_PARAMETER;
    } 
    else if (tick == VM_TIMEOUT_IMMEDIATE)
    {
      //TODO something....
    }
    allThreads[curID].t_ticks = tick;
    allThreads[curID].t_state = VM_THREAD_STATE_WAITING;
    sleeping.push_back(allThreads[curID]);
    scheduler();
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