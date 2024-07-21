/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/
ContFramePool* ContFramePool::head = nullptr;
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
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;

    if (_info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (_info_frame_no * FRAME_SIZE);
    }
    
    assert((nframes%8) == 0);
    
    BitmapInit(_info_frame_no);
    ptrHeadToNext();

    Console::puts("initialized\n");
}

//helper function: BitmapInit() which initialized bitmap
void ContFramePool::BitmapInit(unsigned long _info_frame_no) {
     int start = 0;
     if (_info_frame_no == 0){
        bitmap[0] = 0x7F;
        nFreeFrames--;
        start = 1;
     }
     
     for (int i = start; i < nframes; i++){
          bitmap[i] = 0xFF;
     }
}

//helper function: ptrHeadToNext
void ContFramePool::ptrHeadToNext(){
     ContFramePool** temp = &head;
     while(*temp){
           temp = &(*temp)->next;
     }
     
     *temp = this;
     this->next = NULL;
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{

    if (_n_frames > nframes || _n_frames > nFreeFrames){
       Console::puts("ERROR:Allocate frames are more than available frames\n");
       assert(false);
    }
    
    for (unsigned int i = 0; i <= nframes - _n_frames; i++){
        if (checkFrame(i, _n_frames)) {
           checkBitmap(i, _n_frames);
           return base_frame_no + i;
        }
    } 
    Console::puts("ERROR: Free Frame may not continuous\n");
    assert(false);
    return 0;

}

//helper function: checkFrame() which is check available frame
bool ContFramePool::checkFrame(unsigned index, unsigned int _n_frames){
     unsigned int j;
     for(j = index; j < index + _n_frames; j++){
        if (bitmap[j] == 0x7F || bitmap[j] == 0x3F) {
           return false;
        }
     }
     return true;
}

//helper function: checkBitmap()
void ContFramePool::checkBitmap(unsigned int i, unsigned int _n_frames){
     unsigned char mask = 0x80;
     unsigned int k;
     for(k = 0; k < _n_frames; k++){
        bitmap[i] = (k==0) ? (bitmap[i] ^ 0xC0) : (bitmap[i] ^ mask);
        nFreeFrames--;
        i++;
     }
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
                                      
{
     unsigned long frame_mark = _base_frame_no + _n_frames;
     unsigned char mask = 0xC0;
     
     if(_base_frame_no < base_frame_no || frame_mark > base_frame_no + nframes){
        Console::puts("OUT OF BOUNDS");
        assert(false);
     }
     for(unsigned long i = _base_frame_no; i < frame_mark; i++){
         unsigned int bitmap_i = i - base_frame_no;
         assert((bitmap[bitmap_i] & mask) !=0);
         bitmap[bitmap_i] ^= mask;
     }
     nFreeFrames -= _n_frames;
}


void ContFramePool::release_frames(unsigned long _first_frame_no)
{
     auto frame_range = [](ContFramePool *pool, unsigned long frame_no){
          return frame_no >= pool->base_frame_no && frame_no <= pool->base_frame_no + pool->nframes -1;
     };
     
     for(ContFramePool *temp = head; temp != nullptr; temp = temp->next) {
        if(frame_range(temp, _first_frame_no)){
           if(temp-> bitmap[_first_frame_no -temp->base_frame_no] == 0x3F){
              int i = _first_frame_no - temp->base_frame_no;
              for(i; i < temp->nframes && (temp->bitmap[i] ^ 0xFF) == 0x80; i++){
                  temp->bitmap[i] |= 0xC0;
                  temp->nFreeFrames++;
              } 
           
           } else {
               Console::puts("NOT Head Frame");
               assert(false);
           }
        break;
        }
     }
}



unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    const unsigned long bit_per_frame = 8;
    const unsigned long stored = 4*1024*8;
    unsigned long total = _n_frames * bit_per_frame;
    unsigned long result = total/stored;
    unsigned long remainder = total%stored; 

    
    return (remainder>0) ? result + 1 : result;
    
}
