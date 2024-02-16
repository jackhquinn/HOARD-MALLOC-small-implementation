//
// Created by Clay Shields on 8/25/22.
// CHATGPT Links: https://chat.openai.com/share/72f25cb8-7d44-4518-bbd0-2406d5327ead
// https://chat.openai.com/share/ac73a9b6-f81e-4940-bc3d-d4a000d7f64c

#include "malloc.h"
#include <iostream>
#include <sys/mman.h>
#define SUPERBLOCK_MASK (~(SUPERBLOCKSIZE-1)) // make the mask

using namespace std;

static bool initialized = false;
blockList* alloc_sizes[BUCKETS];

void initializeAllocSizes() {
    if (!initialized) {
        static blockList actual_blocks[BUCKETS];
        for (int i = 0; i < BUCKETS; i++) {
            alloc_sizes[i] = &actual_blocks[i];
            alloc_sizes[i]->header = nullptr;
            alloc_sizes[i]->freeCount = 0;
            alloc_sizes[i]->superBlockNum = 0;
        }
        initialized = true;
    }
}

int getIndexForSize(size_t size) {
    const size_t allocationSizes[] = {32, 64, 128, 256, 512, 1024, 2048};
    for (int i = 0; i < BUCKETS; i++) {
        if (size <= allocationSizes[i]) {
            return i;
        }
    }
    return -1;
}

size_t getSizeForIndex(int index) {
    const size_t allocationSizes[] = {32, 64, 128, 256, 512, 1024, 2048};

    // Ensure that the index is within bounds.
    if (index >= 0 && index < BUCKETS) {
        return allocationSizes[index];
    } else {
        // Handle an invalid index (out of bounds).
        return 0; // You can return a default or error value here.
    }
}

SuperBlockHeader* allocateBlock(size_t allocationSize, short index) {
    // Call mmap to allocate a new Superblock
    void *blockMemory = mmap(nullptr, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (blockMemory == MAP_FAILED) {
        return nullptr; // mmap failed
    }

    SuperBlockHeader* newHeader = (SuperBlockHeader*) blockMemory;
    newHeader->nextSuperblockHeader = nullptr;
    newHeader->freeList = nullptr;
    newHeader->freeCount = 0;
    newHeader->superBlockSize = allocationSize;

    // Calculate the address where the first Alloc should be
    unsigned char* firstAddress = (unsigned char*)(newHeader + 1);

    newHeader->freeList = (Alloc*)firstAddress;

    // Create a linked list of allocs in the free space of the Superblock
    for (int i = 0; i < (SUPERBLOCKSIZE) / allocationSize - 1; ++i) {
        unsigned char* allocAddress = firstAddress + (i * allocationSize);
        Alloc* newAlloc = (Alloc*)(allocAddress);
        newAlloc->next = (Alloc*)(allocAddress + allocationSize);
    }

    // the next of the last alloc is null
    unsigned char* lastAddress = firstAddress + ((SUPERBLOCKSIZE) / allocationSize - 1) * allocationSize;
    Alloc* lastAlloc = (Alloc*) lastAddress;
    lastAlloc->next = nullptr;

    // Link the new superblock to the front of the superblock list
    newHeader->nextSuperblockHeader = alloc_sizes[index]->header;
    alloc_sizes[index]->header = newHeader;

    return newHeader;
}

void updateNums(SuperBlockHeader* superblockHeader, size_t allocationSize, short listIndex){
    // Update the free count for the newly created superblock
    //cout << "initializing SB freecount." << endl;
    superblockHeader->freeCount = (SUPERBLOCKSIZE) / allocationSize - 1;

    // Increase the total free count for the superblock list
    // cout << "incrementing SB freecount." << endl;
    alloc_sizes[listIndex]->freeCount += superblockHeader->freeCount;
    alloc_sizes[listIndex]->superBlockNum++; // increment the count of superblocks for that index

    // Increment the number of superblocks in the list
    alloc_sizes[listIndex]->superBlockNum++;
}

void * mmalloc (size_t size) {

    // cout << "alloc request for " << size << endl;

    // if the array is not initilaized, initialize the alloc sizes array
    if(!initialized)
        initializeAllocSizes();

    //if(size >= PAGESIZE / 2) { cout << "Size >= page size/2. Using system malloc." << endl; return malloc(size); }

    // return the index of the array which correspondes to the rounded size
    uint32_t index = getIndexForSize(size);
    uint32_t roundedSize = getSizeForIndex(index);

    SuperBlockHeader* header = alloc_sizes[index]->header;
    //cout << "Got header: " <<  hex << header << dec << endl;

    if ((header == nullptr) || (alloc_sizes[index]->freeCount == 0)) {
        // Allocate superblock if there's no free space
        //cout << "allocating a block "<< endl;
        header = allocateBlock(roundedSize, index);
        if (!header) {
            return nullptr; // Allocation failed
        }
        updateNums(header, roundedSize, index);
    } else {
        // cout << "alloc_sizes[index]->freeCount: " << alloc_sizes[index]->freeCount << endl;
        // cout << "traversing list  " << index << endl;
        while (header && header->freeCount <= 0) {
            header = header->nextSuperblockHeader;
        }
        // If, after this loop, header is null, then there's an inconsistency in free count
    }

    //cout << "if this is null we will crash" << hex << header << dec << endl;

    if (header == nullptr) {
        cout << "Die before crashing. " << endl;
        exit(-1);
    }

    // find the next free allocation
    unsigned char* freeAddress = (unsigned char*) (header->freeList);

    // Adjust the superblock's free list and free count
    Alloc* data = (Alloc*)freeAddress;
    header->freeList = data->next;
    // cout << "decrementing SB freeCount" << endl;
    header->freeCount--;
    // cout << "decrementing alloc sizes freeCount" << endl;
    alloc_sizes[index]->freeCount--;
    // cout << "header later is: " << hex << header << dec << endl;
    // cout << "free count now is: " << header->freeCount << endl;

    if (header->freeCount < 0) {
        cout << "Negative header free count" << endl;
        exit(-1);
    }

    if (alloc_sizes[index]->freeCount < 0){
        cout << "Negative alloc sizes free count" << endl;
        exit(-1);
    }

    //cout << "returning " << hex << data << dec << endl;
    return data;
}

void deleteSuperBlock(SuperBlockHeader* superblock, int index){
    int numFree = superblock->freeCount;

    // Unlink the superblock from the list
    if (alloc_sizes[index]->header == superblock) {
        // If the superblock is the first in the list
        alloc_sizes[index]->header = superblock->nextSuperblockHeader;
    } else {
        SuperBlockHeader* curr = alloc_sizes[index]->header;
        SuperBlockHeader* lag = nullptr;  // Initialize lag to nullptr since it starts off not pointing to any node
        while (curr != superblock) {
            lag = curr;
            curr = curr->nextSuperblockHeader;
        }
        // Curr points to the superblock we want to remove and lag points to the superblock just before it
        if (lag) {  // Check to make sure lag is not nullptr (this should always be true)
            lag->nextSuperblockHeader = curr->nextSuperblockHeader;
        }
    }

    alloc_sizes[index]->freeCount -= numFree;
    alloc_sizes[index]->superBlockNum--; // decrement the count of superblocks for that index

    // Free the superblock's memory
    munmap(superblock, PAGESIZE);
}

void mfree (void * ptr) {

    //cout << "Clay: dealloc request for " << ptr << endl;

    // figure out which page the pointer is in
    uint64_t base = (uint64_t) ptr; // convert the pointer to an int to allow masking
    base &= SUPERBLOCK_MASK; // mask away

    SuperBlockHeader* tempheader = (SuperBlockHeader*) base;

    /*
    if (tempheader->superBlockSize > PAGESIZE / 2) {
        cout << "Size > page size/2. Using system free." << endl;
        free(ptr);
        return; // Assuming this function returns void.
    }*/

    int alloc_index = getIndexForSize(tempheader->superBlockSize);

    // Put the pointer back into the free list
    Alloc* data = (Alloc*)ptr;
    data->next = tempheader->freeList;
    tempheader->freeList = data;

    // Update the free counts
    tempheader->freeCount++;
    alloc_sizes[alloc_index]->freeCount++;

    // If there are more than 4 free superblocks and all allocs in this superblock are free, delete the superblock.
    if (alloc_sizes[alloc_index]->superBlockNum > RETAIN_FREE_SUPERBLOCK_COUNT &&
        tempheader->freeCount == (SUPERBLOCKSIZE) / tempheader->superBlockSize - 1) {
        deleteSuperBlock(tempheader, alloc_index);
    }
}

void mstats(){
    cout << "Memory Allocation Statistics:" << endl;

    for (int i = 0; i < BUCKETS; i++) {
        if (alloc_sizes[i]->superBlockNum > 0) {
            // Calculate the allocation size for this bucket
            size_t allocationSize = getSizeForIndex(i);

            int numAllocsPerSuperblock = PAGESIZE / allocationSize;
            int freeSuperblocksForBucket = alloc_sizes[i]->freeCount / numAllocsPerSuperblock;

            cout << allocationSize << "B allocations:" << endl;
            cout << " - Superblocks allocated: " << alloc_sizes[i]->superBlockNum << endl;
            cout << " - Superblocks free: " << freeSuperblocksForBucket << endl;
            cout << " - Total allocated: " << (alloc_sizes[i]->superBlockNum * PAGESIZE / allocationSize) << endl;
            cout << " - Allocations free: " << alloc_sizes[i]->freeCount << endl;
        }
    }

    /*
    // Calculate unused allocated space
    size_t unusedAllocatedSpace = 0;
    for (int i = 0; i < BUCKETS; i++) {
        size_t allocationSize = getSizeForIndex(i);
        unusedAllocatedSpace += (alloc_sizes[i]->superBlockNum * PAGESIZE) -
                                (alloc_sizes[i]->freeCount * allocationSize);
    }
    cout << "Unused allocated space: " << unusedAllocatedSpace << "B" << endl;

    // Calculate cumulative internal fragmentation
    size_t totalAllocatedSpace = 0;
    for (int i = 0; i < BUCKETS; i++) {
        totalAllocatedSpace += alloc_sizes[i]->superBlockNum * PAGESIZE;
    }
    double cumulativeFragmentation = (double)unusedAllocatedSpace / totalAllocatedSpace;
    cout << "Cumulative internal fragmentation: " << cumulativeFragmentation * 100 << "%" << endl;
     */

}
