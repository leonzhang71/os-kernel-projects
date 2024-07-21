/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "simple_timer.H"
/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  head = nullptr;
  tail = nullptr;
  //assert(false);
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
    
    bool areInterruptsEnabled = Machine::interrupts_enabled();
    if (areInterruptsEnabled) {
        Machine::disable_interrupts();
    }
    
    ThreadNode *tempQueue = head;
    if (tempQueue) {
        Thread *schedThread = tempQueue->thread;
        head = tempQueue->next;
        tail = (!head) ? nullptr : tail;

        delete tempQueue;
        Thread::dispatch_to(schedThread);
    }
    
    if (!areInterruptsEnabled) {
        Machine::enable_interrupts();
    } 
  //assert(false);
}

void Scheduler::resume(Thread * _thread) {
    
    bool areInterruptsEnabled = Machine::interrupts_enabled();
    if (areInterruptsEnabled) {
        Machine::disable_interrupts();
    } 
    
    ThreadNode *newEntry = new ThreadNode;
    newEntry->thread = _thread;
    newEntry->next = nullptr; 

    if (!head) { 
        head = newEntry;
    } else {
        tail->next = newEntry; 
    }
    tail = newEntry;
    
    if (!areInterruptsEnabled) {
        Machine::enable_interrupts();
    } 
  //assert(false);
}

void Scheduler::add(Thread * _thread) {
  resume (_thread);
  //assert(false);
}

void Scheduler::terminate(Thread * _thread) {
    delete _thread;
    yield ();
  //assert(false);
}
