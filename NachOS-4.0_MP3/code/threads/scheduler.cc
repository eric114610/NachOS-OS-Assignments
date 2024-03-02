// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improv ed in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

int comp_l1(Thread *a, Thread* b){     // mp3
    float a_rm_approx_bt = a -> approx_burst_time - a -> accumulated_ticks;
    float b_rm_approx_bt = b -> approx_burst_time - b -> accumulated_ticks;
    if(a_rm_approx_bt < b_rm_approx_bt)return -1;
    else if(a_rm_approx_bt == b_rm_approx_bt)return 0;
    else return 1;
}
int comp_l2(Thread *a, Thread* b){ //QUES
    if(a -> priority < b -> priority)return 1;
    else if(a -> priority == b -> priority)return 0;
    else return -1;
}
Scheduler::Scheduler()
{ 
    readyList = new List<Thread *>; 

    // mp3
    readyList_l1 = new SortedList<Thread *>(comp_l1); 
    readyList_l2 = new SortedList<Thread *>(comp_l2); 
    readyList_l3 = new List<Thread *>;  
    // pending_aging = new SortedList<Thread *>(comp_aging);

    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 

    delete readyList_l1;
    delete readyList_l2;
    delete readyList_l3; // mp3
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);
    // readylist->Append(thread);

    // mp3
    Statistics *stats = kernel -> stats;
    int toque;
    if(thread -> priority >= 100){                
        readyList_l1->Insert(thread);  
        toque = 1;
    }else if(thread -> priority >= 50){
        readyList_l2->Insert(thread);
        toque = 2;
    }else {
        readyList_l3->Append(thread);
        toque = 3;
    }
    // pending_aging -> Insert(thread);
    DEBUG(dbgProc, "[A] Tick [" << stats -> totalTicks << "]: Thread [" << thread -> getID() << "] is inserted into queue L[" << toque << "]");
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    
    // if (readyList->IsEmpty()) {
	// 	return NULL;
    // } else {
    // 	return readyList->RemoveFront();
    // }

    // mp3
    // aging();
    Thread *nextThread = 0;
    Statistics *stats = kernel -> stats;
    int fromque = 0;
    if(!readyList_l1 -> IsEmpty()){
        nextThread = readyList_l1 -> RemoveFront();
        fromque = 1;
    }else if(!readyList_l2 -> IsEmpty()){
        nextThread = readyList_l2 -> RemoveFront();
        fromque = 2;
    }else if(!readyList_l3 -> IsEmpty()){
        nextThread = readyList_l3 -> RemoveFront();
        fromque = 3;
    }else{
        nextThread = NULL;
        fromque = 0;
    }
    if(fromque){
        DEBUG(dbgProc, "[B] Tick [" << stats -> totalTicks << "]: Thread [" << nextThread -> getID() << "] is removed from queue L[" << fromque << "]");
    }
    // pending_aging -> Remove(nextThread);
    return nextThread;
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    Statistics *stats = kernel -> stats; //mp3
    DEBUG(dbgThread, "Switching from: " << oldThread->getID() << " to: " << nextThread->getID());
    int tmp;
    if(oldThread -> accumulated_ticks == 0) tmp = oldThread -> last_accumulated_ticks;
    else tmp = oldThread -> accumulated_ticks;
    DEBUG(dbgProc, "[E] Tick [" << stats -> totalTicks << "]: Thread [" << nextThread -> getID() 
                            << "] is now selected for execution, thread [" << oldThread -> getID() 
                            << "] is replaced, and it has executed [" << tmp << "] ticks");
    
    // oldThread -> accumulated_ticks = 0; 

    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".
    #ifdef x86
    //DEBUG(dbgDisk, "diskk");
    # endif

    SWITCH(oldThread, nextThread);
    

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
//  Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList->Apply(ThreadPrint);
}

//----------------------------------------------------------------------
//  Scheduler::check_preempt
// 	check if the current process needs to be preempted from alarm
//  mp3
//----------------------------------------------------------------------
bool Scheduler::check_preempt(){

    Thread *nowThread = kernel -> currentThread;
    int nowque;
    if(nowThread->priority >= 100)nowque = 1;
    else if(nowThread -> priority >= 50)nowque = 2;
    else nowque = 3;
    // DEBUG(dbgProc, "check preempt");
    
    if(!readyList_l1 -> IsEmpty()){                        // l1 preempts everyone
        if(nowque == 1){
            float a_rm_approx_bt = readyList_l1 -> Front() -> approx_burst_time - readyList_l1 -> Front() -> accumulated_ticks;
            float b_rm_approx_bt = nowThread -> approx_burst_time - (nowThread -> accumulated_ticks + kernel -> stats -> totalTicks - nowThread -> burst_start_time);
            // DEBUG(dbgProc, "l1 front : thread " << readyList_l1 -> Front() -> getID() << ", rm bt = " << a_rm_approx_bt);
            // DEBUG(dbgProc, "now thread " << nowThread -> getID() << ", rm bt = " << b_rm_approx_bt);
            if(a_rm_approx_bt < b_rm_approx_bt)return true;
        }else return true;

    }else if(!readyList_l2 -> IsEmpty() && nowque == 3){   // l2 preempts l3
        return true;
    }else if(!readyList_l3 -> IsEmpty() && nowque == 3 
             && kernel -> stats -> totalTicks - nowThread -> burst_start_time >= 100){   // l3 round robin
        return true;
    }
    // else if(readyList_l1 -> IsEmpty() && readyList_l2 -> IsEmpty() && readyList_l3 -> IsEmpty())
    //     return true;
    return false;
}

//----------------------------------------------------------------------
//  Scheduler::aging
// 	implment the aging mechanism
//  priority + 10 ever 1500 ticks
//  mp3
//----------------------------------------------------------------------
void Scheduler::aging(){

    Statistics *stats = kernel -> stats;

    ListIterator<Thread *> it1(readyList_l1);
    while( !it1.IsDone()){
        Thread *nowThread = it1.Item();
        if(stats -> totalTicks - nowThread -> wait_tick >= 1500){
            nowThread -> add_priority(10);
            nowThread -> wait_tick = stats -> totalTicks;
        }
        it1.Next();
    }

    ListIterator<Thread *> it2(readyList_l2);
    while( !it2.IsDone()){
        Thread *nowThread = it2.Item();
        if(stats -> totalTicks - nowThread -> wait_tick >= 1500){
            nowThread -> add_priority(10);
            nowThread -> wait_tick = stats -> totalTicks;
            if(nowThread -> priority >= 100){
                readyList_l2 -> Remove(nowThread);
                DEBUG(dbgProc, "[B] Tick [" << stats -> totalTicks << "]: Thread [" << nowThread -> getID() << "] is removed from queue L[2]");
                readyList_l1 -> Insert(nowThread);
                DEBUG(dbgProc, "[A] Tick [" << stats -> totalTicks << "]: Thread [" << nowThread -> getID() << "] is inserted into queue L[1]");
            }
        }
        it2.Next();
    }

    ListIterator<Thread *> it3(readyList_l3);
    while( !it3.IsDone()){
        Thread *nowThread = it3.Item();
        if(stats -> totalTicks - nowThread -> wait_tick >= 1500){
            nowThread -> add_priority(10);
            nowThread -> wait_tick = stats -> totalTicks;
            if(nowThread -> priority >= 50){
                readyList_l3 -> Remove(nowThread);
                DEBUG(dbgProc, "[B] Tick [" << stats -> totalTicks << "]: Thread [" << nowThread -> getID() << "] is removed from queue L[3]");
                readyList_l2 -> Insert(nowThread);
                DEBUG(dbgProc, "[A] Tick [" << stats -> totalTicks << "]: Thread [" << nowThread -> getID() << "] is inserted into queue L[2]");
            }
        }
        it3.Next();
    }
}