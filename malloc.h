//
// Created by Clay Shields on 8/25/22.
//

#ifndef MALLOC_MALLOC_H
#define MALLOC_MALLOC_H
#include<stdio.h>

#define MIN_ALLOC 32
#define MIN_ALLOC_POWER 5
#define PAGESIZE 4096
#define SUPERBLOCKSIZE 4096
#define RETAIN_FREE_SUPERBLOCK_COUNT 4
#define BUCKETS 7

struct Alloc {
public:
    Alloc* next; // pointer to the next free allocation
};

struct SuperBlockHeader {
    SuperBlockHeader* nextSuperblockHeader; // Next superblock header
    Alloc* freeList; // Pointer to the next free allocation
    short superBlockSize; // size of the superblock
    short freeCount; // number of free allocations
};

struct blockList {
    SuperBlockHeader* header; // pointer to the superblockheaders within that blocklist
    short freeCount; // sum of all free allocations in the superblock list
    short superBlockNum; // number of superblocks in the list
};

void initializeAllocSizes();

int getIndexForSize(size_t size);
size_t getSizeForIndex(int index);
SuperBlockHeader* allocateBlock(size_t allocationSize, short index);
void updateNums(SuperBlockHeader* superblockHeader, size_t allocationSize, short listIndex);
void * mmalloc (size_t __size);

void deleteSuperBlock(SuperBlockHeader* superblock, int index);
void mfree(void*);

void mstats();

#endif //MALLOC_MALLOC_H


/*
struct SuperBlock {
    SuperBlockHeader* header;
};*/

/*

 while(header){
     if(header->freeCount > 0) {
         // find the next free allocation
         unsigned char* freeAddress = (unsigned char*) (header + sizeof(SuperBlockHeader) +
                                                                 (PAGESIZE - header->freeCount * size));
         // Cast the address to an Alloc* and use it as the new allocation
         data = (Alloc*)freeAddress;

         // Update the free list with the new allocation
         data->next = header->freeList;
         header->freeList = data;
         header->freeCount--;

         break;
     } else {
         header = header->nextSuperblockHeader;
     }
 }*/