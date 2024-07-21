/*
     File        : blocking_disk.H

     Author      : 

     Date        : 
     Description : 

*/

#ifndef _BLOCKING_DISK_H_
#define _BLOCKING_DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"
#include "thread.H"
#include "interrupts.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* B l o c k i n g D i s k  */
/*--------------------------------------------------------------------------*/

class BlockingDisk : public SimpleDisk, public InterruptHandler {
private:
    typedef struct queue {
        Thread * thread;
        struct queue * next;
    } ThreadNode;

    bool lock_state;
    ThreadNode * head_ptr;
    ThreadNode * tail_ptr;

    bool check_lock();
    void set_lock();
    void set_unlock();
    Thread * dequeue();
    void enqueue(Thread * _thread);

protected:
    void wait_until_ready() override;
    
public:
   BlockingDisk(DISK_ID _disk_id, unsigned int _size); 
   /* Creates a BlockingDisk device with the given size connected to the 
      MASTER or SLAVE slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness. 
      In a real system, we would infer this information from the 
      disk controller. */

   /* DISK OPERATIONS */

   virtual void read(unsigned long _block_no, unsigned char * _buf) override;
   /* Reads 512 Bytes from the given block of the disk and copies them 
      to the given buffer. No error check! */

   virtual void write(unsigned long _block_no, unsigned char * _buf) override;
   /* Writes 512 Bytes from the buffer to the given block on the disk. */
   
   void handle_interrupt(REGS *_r) override;
};

#endif
