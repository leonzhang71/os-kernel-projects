/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
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
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    Console::puts("Opening file with id = ");
    Console::puti(_id);
    Console::puts("\n");
    fs = _fs;
    position = 0;
    block_cache = new unsigned char[SimpleDisk::BLOCK_SIZE];
    edit_block_cache = false;
    inode = _fs->LookupFile(_id);

    if (!_fs) {
        Console::puts("Error: null pointer exist\n");
        assert(false);
     }

    if (!inode) {
        Console::puts("Error: Need to find File\n");
        return;
    }
}

File::~File() {
    if (fs && inode) {
        // Output file closure message
        Console::puts("Closing file with id = ");
        Console::puti(inode->id);
        Console::puts("\n");

        // Handle modified block cache
        if (edit_block_cache) {
            // Calculate block index and determine the appropriate block to write
            unsigned int blockIndex = position / SimpleDisk::BLOCK_SIZE;
            unsigned int targetBlock = blockIndex < NUM_DIRECT_BLOCKS ? 
                                       inode->direct_blocks[blockIndex] : 
                                       inode->indirect_block;
            // Write the block to the file system
            fs->WriteBlock(targetBlock, block_cache);
        }

         // Clean up block cache
         delete[] block_cache;

         // Update inode information
         fs->StoreInode(inode);
    }
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("Reading file with id = ");Console::puti(inode->id);Console::puts("\n");
    
    int bytesRead = 0;
    while (_n > 0 && !EoF()) {
        bytesRead += readNextBlock(_n, _buf + bytesRead);
    }

    Console::puts("Finished reading ");Console::puti(bytesRead);Console::puts(" bytes \n");

    return bytesRead;
}

int File::readNextBlock(unsigned int &n, char *destination) {
    unsigned int blockIndex = position / SimpleDisk::BLOCK_SIZE;
    unsigned int offset = position % SimpleDisk::BLOCK_SIZE;
    int bytesToRead = (n < SimpleDisk::BLOCK_SIZE - offset) ? n : SimpleDisk::BLOCK_SIZE - offset;

    loadAndReadBlock(blockIndex, offset, bytesToRead, destination);

    position += bytesToRead;
    n -= bytesToRead;
    return bytesToRead;
}

void File::loadAndReadBlock(unsigned int blockIndex, unsigned int offset, int bytesToRead, char *destination) {
    if (blockIndex < NUM_DIRECT_BLOCKS) {
        fs->ReadBlock(inode->direct_blocks[blockIndex], block_cache);
    } else {
        loadIndirectBlock(blockIndex);
    }
    memcpy(destination, block_cache + offset, bytesToRead);
    edit_block_cache = false;
}

void File::loadIndirectBlock(unsigned int blockIndex) {
    unsigned int indirectIndex = (blockIndex - NUM_DIRECT_BLOCKS) / ippb;
    unsigned int indirectOffset = (blockIndex - NUM_DIRECT_BLOCKS) % ippb;
    
    auto indirectBlock = new unsigned char[SimpleDisk::BLOCK_SIZE];
    fs->ReadBlock(inode->indirect_blocks[indirectIndex], indirectBlock);

    unsigned long dataBlockNumber;
    memcpy(&dataBlockNumber, indirectBlock + indirectOffset * sizeof(unsigned long), sizeof(unsigned long));
    delete[] indirectBlock;

    fs->ReadBlock(dataBlockNumber, block_cache);
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("Writing file with id = ");Console::puti(inode->id);Console::puts("\n");
    unsigned int bytes_written = 0, remaining_bytes = _n;
    unsigned int block_index, offset_in_block, bytes_to_write;
    auto *indirect_block = new unsigned int[ippb];

    while (remaining_bytes > 0) {
        CalculateBlockPosition(position, block_index, offset_in_block);

        bytes_to_write = MIN(SimpleDisk::BLOCK_SIZE - offset_in_block, remaining_bytes);

        if (HandleBlockSizeLimit(block_index)) break;

        unsigned int block_to_read = DetermineBlockToRead(block_index, indirect_block);
        if (block_to_read == 0) {
            block_to_read = AllocateNewBlock(block_index, indirect_block);
            if (block_to_read == 0) break;
        }

        ProcessWrite(block_to_read, offset_in_block, _buf, bytes_written, bytes_to_write);

        UpdatePosition(bytes_to_write, remaining_bytes, bytes_written);
    }

    delete[] indirect_block;
    Console::puts("Finished writing ");Console::puti(bytes_written);Console::puts(" bytes\n");
    return bytes_written;
}
void File::CalculateBlockPosition(unsigned int position, unsigned int &block_index, unsigned int &offset_in_block) {
    block_index = position / SimpleDisk::BLOCK_SIZE;
    offset_in_block = position % SimpleDisk::BLOCK_SIZE;
}

bool File::HandleBlockSizeLimit(unsigned int block_index) {
    if (block_index >= NUM_DIRECT_BLOCKS + ippb) {
        Console::puts("Error: File size limit reached\n");
        return true;
    }
    return false;
}

unsigned int File::DetermineBlockToRead(unsigned int block_index, unsigned int *indirect_block) {
    if (block_index < NUM_DIRECT_BLOCKS) {
        Console::puts("Writing to direct block\n");
        return inode->direct_blocks[block_index];
    } else {
        HandleIndirectBlock(block_index, indirect_block);
        return indirect_block[block_index - NUM_DIRECT_BLOCKS];
    }
}

void File::HandleIndirectBlock(unsigned int block_index, unsigned int *indirect_block) {
    Console::puts("Writing to indirect block\n");
    if (inode->indirect_block == 0) {
        inode->indirect_block = fs->BlockFree();
        if (inode->indirect_block == 0) {
            Console::puts("Error: No free blocks left for indirect block\n");
            return;
        }
        memset(indirect_block, 0, sizeof(indirect_block));
        fs->WriteBlock(inode->indirect_block, (unsigned char *)indirect_block);
    }
    fs->ReadBlock(inode->indirect_block, (unsigned char *)indirect_block);
}

unsigned int File::AllocateNewBlock(unsigned int block_index, unsigned int *indirect_block) {
    unsigned int new_block = fs->BlockFree();
    if (new_block == 0) {
        Console::puts("Error: No free blocks left for data\n");
        return 0;
    }

    UpdateBlockAllocation(block_index, indirect_block, new_block);
    return new_block;
}

void File::UpdateBlockAllocation(unsigned int block_index, unsigned int *indirect_block, unsigned int new_block) {
    if (block_index < NUM_DIRECT_BLOCKS) {
        inode->direct_blocks[block_index] = new_block;
    } else {
        indirect_block[block_index - NUM_DIRECT_BLOCKS] = new_block;
        fs->WriteBlock(inode->indirect_block, (unsigned char *)indirect_block);
    }
}

void File::ProcessWrite(unsigned int block_to_read, unsigned int offset_in_block, const char *_buf, unsigned int &bytes_written, unsigned int &bytes_to_write) {
    fs->ReadBlock(block_to_read, block_cache);
    memcpy(block_cache + offset_in_block, _buf + bytes_written, bytes_to_write);
    fs->WriteBlock(block_to_read, block_cache);
    edit_block_cache = true;
}

void File::UpdatePosition(unsigned int &bytes_to_write, unsigned int &remaining_bytes, unsigned int &bytes_written) {
    position += bytes_to_write;
    remaining_bytes -= bytes_to_write;
    bytes_written += bytes_to_write;
    if (position > inode->size) {
        inode->size = position;
    }
}

void File::Reset() {
    Console::puts("resetting file\n");
    position = 0;
}

bool File::EoF() {
    Console::puts("checking for EoF\n");
    return position >= inode->size;
}
