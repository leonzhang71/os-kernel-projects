/* 
    File: file_system.H

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 21/11/28

    Description: Simple File System.
    

*/

#ifndef _FILE_SYSTEM_H_ // include file only once
#define _FILE_SYSTEM_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

class FileSystem;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

class Inode {
    friend class FileSystem; // The inode is in an uncomfortable position between
    friend class File;       // File System and File. We give both full access
    // to the Inode.
public:
    static constexpr unsigned int NUM_DIRECT_BLOCKS = 1;
    static constexpr unsigned int NUM_INDIRECT_BLOCKS = 1;
    unsigned int direct_blocks[NUM_DIRECT_BLOCKS];
    unsigned int indirect_block;
    unsigned long indirect_blocks[NUM_INDIRECT_BLOCKS];


private:
    long id; // File "name"
    unsigned int size; // Size of the file
    unsigned int block; // Starting block of the file

    FileSystem *fs; // It may be handy to have a pointer to the File system.
    // For example when you need a new block or when you want
    // to load or save the inode list. (Depends on your
    // implementation.)

    /* You may need a few additional functions to help read and store the
       inodes from and to disk. */
};

/*--------------------------------------------------------------------------*/
/* FORWARD DECLARATIONS */
/*--------------------------------------------------------------------------*/

/* (none) */

/*--------------------------------------------------------------------------*/
/* F i l e S y s t e m  */
/*--------------------------------------------------------------------------*/

class FileSystem {

    friend class Inode;

private:
    SimpleDisk *disk;
    unsigned int size;
    //max inode
    static constexpr unsigned int MAX_N = SimpleDisk::BLOCK_SIZE / sizeof(Inode);
    // indirect pointer per block
    const unsigned int ippb = 512 / sizeof(unsigned int);
    //start block for inode
    static constexpr unsigned int inode_start = 1;
    Inode *inodes;

    unsigned char *free_blocks;

    /* The free-block list. You may want to implement the "list" as a bitmap.
       Feel free to use an unsigned char to represent whether a block is free or not;
       no need to go to bits if you don't want to.
       If you reserve one block to store the "free list", you can handle a file system up to
       256kB. (Large enough as a proof of concept.) */

    short GetFreeInode();
    /* Returns the index of a free inode in the inode list. */

public:
    FileSystem();

    /* Just initializes local data structures. Does not connect to disk yet. */

    ~FileSystem();

    /* Unmount file system if it has been mounted. */

    bool Mount(SimpleDisk *_disk);

    /* Associates this file system with a disk. Limit to at most one file system per disk.
       Returns true if operation successful (i.e. there is indeed a file system on the disk.) */

    static bool Format(SimpleDisk *_disk, unsigned int _size);

    /* Wipes any file system from the disk and installs an empty file system of given size. */

    Inode *LookupFile(int _file_id);

    /* Find file with given id in file system. If found, return its inode.
         Otherwise, return null. */

    bool CreateFile(int _file_id);

    /* Create file with given id in the file system. If file exists already,
       abort and return false. Otherwise, return true. */

    bool DeleteFile(int _file_id);

    /* Delete file with given id in the file system; free any disk block occupied by the file. */


   int BlockFree();

    /* Provides the index of the initial available block within the list of free blocks. */

    void EntireInode();

    /* Creating two functions for distributing available inodes within the inode list and free blocks can be beneficial. These functions can also prove to be valuable for the Inode and File classes. */

    void StoreInode(Inode *inode);
    /* Store the entire inode to disk. */
    
    void ReadBlock(unsigned int block, unsigned char *buffer);
    void WriteBlock(unsigned int block, unsigned char *buffer);
};

#endif
