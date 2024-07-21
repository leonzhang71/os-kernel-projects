#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;
unsigned long shared_frames = 0;


void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = _shared_size;
    shared_frames = shared_size / PAGE_SIZE;
   //assert(false);
   Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
    page_directory = (unsigned long *) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);
    auto *page_table = (unsigned long *) ((kernel_mem_pool->get_frames(1)) * PAGE_SIZE);

    for (int i = 0; i < shared_frames; i++) {
        unsigned long mask = i * PAGE_SIZE;
        page_table[i] = mask | 3;
        page_directory[i] = (i == 0) ? (unsigned long) page_table | 3 : mask | 2;
    }

   //assert(false);
   Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
    current_page_table = this;
    write_cr3((unsigned long) page_directory);
    //assert(false);
    Console::puts("Loaded page table\n");

}

void PageTable::enable_paging()
{
    paging_enabled = 1;
    write_cr0(read_cr0() | 0x80000000);
   //assert(false);
   Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
    if (!(_r->err_code & 1)) {
        unsigned long *dir = (unsigned long *) read_cr3();
        unsigned long dirIndex = (read_cr2() >> 22) & 0x3FF;
        unsigned long pageIndex = (read_cr2() >> 12) & 0x3FF;
        unsigned long *tbl;

        unsigned long entry = dir[dirIndex];
        if (entry & 1) {
            tbl = (unsigned long *) (entry & 0xFFFFF000);
        } else {
            dir[dirIndex] = (kernel_mem_pool->get_frames(1) * PAGE_SIZE) | 3;
            tbl = (unsigned long *) (dir[dirIndex] & 0xFFFFF000);
            unsigned int i = 0;
            while (i < shared_frames) tbl[i++] = 2;
        }
        
        tbl[pageIndex] = (process_mem_pool->get_frames(1) * PAGE_SIZE) | 3;
    }

  //assert(false);
  Console::puts("handled page fault\n");
}

