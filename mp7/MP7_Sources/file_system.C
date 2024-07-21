/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store 
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
    disk = nullptr;

    inodes = new Inode[MAX_N];
    for (int i = 0; i < MAX_N; i++) {
        inodes[i] = Inode();
    }

    free_blocks = new unsigned char[SimpleDisk::BLOCK_SIZE]();
    for (int i = 0; i < SimpleDisk::BLOCK_SIZE; i++) {
        free_blocks[i] = 0;
    }
}

FileSystem::~FileSystem() {
    Console::puts("Unmounting file system\n");

    if (disk) {
        disk->write(0, reinterpret_cast<unsigned char*>(inodes));
        disk->write(1, free_blocks);
    }

    delete[] inodes;
    delete[] free_blocks;

    if (disk) {
        disk = nullptr;
    }
}


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/


bool FileSystem::Mount(SimpleDisk *_disk) {
    if (!_disk) {
        Console::puts("Failed to mount: No disk provided.\n");
        return false;
    }

    Console::puts("Mounting file system from disk\n");
    disk = _disk;
    disk->read(0, reinterpret_cast<unsigned char *>(inodes));
    disk->read(1, free_blocks);

    return true;
}

bool FileSystem::Format(SimpleDisk *_disk, unsigned int _size) {   
    if (_size < 2 || _size > _disk->size()) {
        return false;
    }
    unsigned char* block_node_i = new unsigned char[SimpleDisk::BLOCK_SIZE];
    Inode list_empty_node{};
    list_empty_node.id = -1;
    for (unsigned int i = 0; i < MAX_N; ++i) {
        unsigned int offset = i * sizeof(Inode);
        for (unsigned int j = 0; j < sizeof(Inode); ++j) {
            block_node_i[offset + j] = reinterpret_cast<unsigned char*>(&list_empty_node)[j];
        }
    }
    _disk->write(0, block_node_i);
    delete[] block_node_i;

    unsigned char* block_free_list = new unsigned char[SimpleDisk::BLOCK_SIZE];
    for (unsigned int i = 0; i < SimpleDisk::BLOCK_SIZE; ++i) {
        block_free_list[i] = (i < 2) ? 0x00 : 0xFF; 
    }
    
    unsigned int need_size = _size * 1024;

    unsigned int num_blocks = need_size / SimpleDisk::BLOCK_SIZE;
    for (unsigned int i = num_blocks; i < SimpleDisk::BLOCK_SIZE; ++i) {
        block_free_list[i] = 0x00; 
    }

    _disk->write(1, block_free_list);
    delete[] block_free_list;

    return true;
}

Inode *FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = ");
    Console::puti(_file_id);
    Console::puts("\n");
    unsigned int i = 0;
    while (i < MAX_N) {
          if (inodes[i].id == _file_id) {
              return &inodes[i];
          }
          ++i;
    }
    return nullptr;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file with id:");
    Console::puti(_file_id);
    Console::puts("\n");    
    if (LookupFile(_file_id) != nullptr) {
        Console::puts("Failure: File already exists.");
        return false;
    }

    // Find a free inode index
    short index_free_node = GetFreeInode();
    if (index_free_node < 0) {
        Console::puts("Failure: All inodes are currently in use.");
        return false;
    }

    // Initialize the inode
    Inode *inode = &inodes[index_free_node];
    inode->id = _file_id;
    inode->size = 0;
    inode->fs = this;

    // block pointers Initialized direct and indirect
    for (auto &block : inode->direct_blocks) {
         block = 0;
    }
    inode->indirect_block = 0;

    // Save the inode list
    EntireInode();

    return true;
}

short FileSystem::GetFreeInode() {
    unsigned int idx = 0;
    while (idx < MAX_N) {
           if (inodes[idx].id == -1) {
               return idx;
           }
           ++idx;
    }
    return -1;
}

int FileSystem::BlockFree() {

    unsigned int counter = 0;
    while (counter < SimpleDisk::BLOCK_SIZE) {
           if (free_blocks[counter] == 0xFF) { // Check if block is free
               free_blocks[counter] = 0x00;    // Set block as used
               return counter;
            }
            ++counter;
     }
return -1;
// No free blocks available
}

bool FileSystem::DeleteFile(int _file_id) {
    Inode *inode = LookupFile(_file_id);

    if (inode == nullptr) {
        return false; 
    }

    for (unsigned int &direct_block : inode->direct_blocks) {
        if (direct_block == 0) {
            continue;
        }

        free_blocks[direct_block] = 0xFF;
        direct_block = 0;
    }

    if (inode->indirect_block != 0) {
        unsigned char indirect_block_cache[SimpleDisk::BLOCK_SIZE];
        //ReadBlock(inode->indirect_block, indirect_block_cache);
        disk->read(inode->indirect_block, indirect_block_cache);

        for (int i = 0; i < ippb; ++i) {
            unsigned int data_block;
            memcpy(&data_block, indirect_block_cache + i * sizeof(unsigned int), sizeof(unsigned int));

            if (data_block == 0) {
                continue;
            }

            free_blocks[data_block] = 0xFF;
        }

        free_blocks[inode->indirect_block] = 0xFF;
        inode->indirect_block = 0;
    }

    inode->size = 0;
    StoreInode(inode);
    inode->id = -1;
    EntireInode();

    return true;
}

void FileSystem::StoreInode(Inode *inode) {
    if (!inode) {
        return;
    }
    unsigned int inode_index = inode->id;
    unsigned int node_offset = inode_index * sizeof(Inode);
    unsigned char block_cache[SimpleDisk::BLOCK_SIZE];
    disk->read(inode_start, block_cache);
    memcpy(block_cache + node_offset, inode, sizeof(Inode));
    disk->write(inode_start, block_cache);
 
}

//Saves the list of entire Inode to the disk.
void FileSystem::EntireInode() {
    unsigned int inode_list_blocks = (MAX_N * sizeof(Inode) + SimpleDisk::BLOCK_SIZE - 1) / SimpleDisk::BLOCK_SIZE;
    for (unsigned int i = 0; i < inode_list_blocks; ++i) {
         unsigned char block_cache[SimpleDisk::BLOCK_SIZE];
         const unsigned char* source_ptr = reinterpret_cast<const unsigned char *>(inodes) + i * SimpleDisk::BLOCK_SIZE;
         memcpy(block_cache, source_ptr, SimpleDisk::BLOCK_SIZE);
         disk->write(inode_start + i, block_cache);
    }
}

void FileSystem::ReadBlock(unsigned int block, unsigned char *buffer) {
    disk->read(block, buffer);
}

void FileSystem::WriteBlock(unsigned int block, unsigned char *buffer) {
    disk->write(block, buffer);
}

