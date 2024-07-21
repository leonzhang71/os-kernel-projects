/*
 File: vm_pool.C
 
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

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"
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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    base_addr = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;

    region_no = 0;
    allocation = (struct allocation_ *) (base_addr);
    page_table->register_pool(this);
    //assert(false);
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    if (!_size) {
        return 0;
    }

    assert(region_no < MAX_REGIONS);

    unsigned long framesRequired = (_size + PageTable::PAGE_SIZE - 1) / PageTable::PAGE_SIZE;

    unsigned long prevRegionEnd = (region_no > 0) ? allocation[region_no - 1].base_addr + allocation[region_no - 1].size : base_addr;
    
    allocation[region_no].base_addr = prevRegionEnd;
    allocation[region_no].size = framesRequired * PageTable::PAGE_SIZE;
    region_no++;

    return prevRegionEnd;
}

void VMPool::release(unsigned long _start_address) {
    int regionIndex = 0;
    while (regionIndex < MAX_REGIONS && allocation[regionIndex].base_addr != _start_address) {
        ++regionIndex;
    }

    assert(regionIndex < MAX_REGIONS);

    unsigned long total_pages = allocation[regionIndex].size / PageTable::PAGE_SIZE;

    for (unsigned long pg = 0; pg < total_pages; pg++) {
        page_table->free_page(_start_address + pg * PageTable::PAGE_SIZE);
    }

    // Shift the allocations after the removed one
    for (int j = regionIndex; j < region_no - 1; j++) {
        allocation[j] = allocation[j + 1];
    }

    --region_no;
    page_table->load();
    //assert(false);
    Console::puts("Memory region has been released.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    bool init_limit = _address >= base_addr;
    bool max_limit = _address < (base_addr + size);
    return init_limit && max_limit;
}


