/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"
#include "simple_disk.H"
#include "thread.H"

extern Scheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
    head_ptr = nullptr;
    tail_ptr = nullptr;
    lock_state = false;
}

bool BlockingDisk::check_lock() {
    bool lock_checked = lock_state;
    lock_state = true;

    return lock_checked;
}

void BlockingDisk::set_lock() {
    while (check_lock()) {
        SYSTEM_SCHEDULER->yield();
    }
}

void BlockingDisk::set_unlock() {
    lock_state = false;
}

void BlockingDisk::enqueue(Thread * _thread) {
    ThreadNode* newNode = new ThreadNode();
    newNode->thread = _thread;
    newNode->next = nullptr;

    if (!head_ptr) {
        head_ptr = newNode;
    } else {
        tail_ptr->next = newNode;
    }

    tail_ptr = newNode;
}

Thread * BlockingDisk::dequeue() {
    if (head_ptr == nullptr) {
        return nullptr;
    }

    ThreadNode* nodeToRemove = head_ptr;
    Thread* thread = nodeToRemove->thread;
    head_ptr = nodeToRemove->next;

    if (head_ptr == nullptr) {
        tail_ptr = nullptr;
    }

    delete nodeToRemove;
    return thread;
}
/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::wait_until_ready() {
    Console::puts("Current readiness state of disk: ");
    Console::putui(static_cast<unsigned int>(this->is_ready()));
    Console::puts("\nAwaiting disk readiness...\n");

    while (!this->is_ready()) {
        #ifdef INTERRUPTS_ENABLED
            this->enqueue(Thread::CurrentThread());
        #else
            SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
        #endif
        SYSTEM_SCHEDULER->yield();
    }

    Console::puts("Disk now ready. Resuming thread.\n");
}


void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
    set_lock();
    SimpleDisk::read(_block_no, _buf);
    set_unlock();
}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
    set_lock();
    SimpleDisk::write(_block_no, _buf);
    set_unlock();
}

void BlockingDisk::handle_interrupt(REGS *_r) {
    Console::puts("Processing disk interrupt\n");
    
    Thread *interruptedThread = dequeue();
    set_unlock();
    
    if (interruptedThread != nullptr) {
        SYSTEM_SCHEDULER->resume(interruptedThread->CurrentThread());
    }

    Console::puts("Handling of interrupt 14 complete\n");
}
