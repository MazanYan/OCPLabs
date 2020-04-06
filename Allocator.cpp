#include "Allocator.h"
#include "MemoryBlockHeader.h"
#include <stdio.h>
using namespace std;

MemoryBlockHeader* Allocator::visitNextBlock(MemoryBlockHeader* block) {
	MemoryBlockHeader* nextBlock = reinterpret_cast<MemoryBlockHeader*>(reinterpret_cast<int*>(block) + block->currSize);
	if (nextBlock > operatedMemoryEnd)
		return operatedMemoryEnd;
	return nextBlock;
}

MemoryBlockHeader* Allocator::visitPrevBlock(MemoryBlockHeader* block) {
	MemoryBlockHeader* prevBlock = reinterpret_cast<MemoryBlockHeader*>(reinterpret_cast<int*>(block) - block->prevSize);
	if (prevBlock < startBlock)
		return startBlock;
	return prevBlock;
}

/*
	Method used in mem_realloc to move the datafrom location to another location
*/
void Allocator::move_data(void* from, void* to, size_t size) {
	int* start = (int*) from + BLOCK_HEADER_SIZE/4;
	int* end = (int*) to + BLOCK_HEADER_SIZE/4;
	for (int i = 0; i < size - BLOCK_HEADER_SIZE/4; i++) {
		start[i] = end[i];
	}
}

/*
	Constructor
*/
Allocator::Allocator(size_t size) {
	normalize(size);
	operatedMemorySize = size;
	int* memorySpace = new int[size];
	startBlock = reinterpret_cast<MemoryBlockHeader*>(&memorySpace[0]);
	startBlock->currSize = size;
	startBlock->used = false;
	startBlock->prevSize = 0;
	operatedMemoryEnd = reinterpret_cast<MemoryBlockHeader*>(&memorySpace[size - 1]);
	operatedMemoryEnd->currSize = 0;
	operatedMemoryEnd->prevSize = size;
	operatedMemoryEnd->used = false;
}

/*
	Check if the block of memory is used
*/
bool Allocator::isFree(MemoryBlockHeader* block) {
	return !block->used;
}

/*
	Merge neighbouring blocks of data after freeing the block between them
*/
void Allocator::merge(MemoryBlockHeader* block) {
	block->used = false;
	MemoryBlockHeader* prevBlock = visitPrevBlock(block);
	MemoryBlockHeader* nextBlock = visitNextBlock(block);
	prevBlock->currSize += block->currSize;
	if (isFree(nextBlock))
		prevBlock->currSize += nextBlock->currSize;
	else
		nextBlock->currSize = prevBlock->currSize;
}

/*
	Allocate memory in a block. The method finds a free block with appropriate size, then splits
	it into free and not free blocks
*/
void* Allocator::mem_alloc(size_t size) {
	normalize(size);
	MemoryBlockHeader* memoryBlockWorkingHeader = startBlock;
	while (memoryBlockWorkingHeader < operatedMemoryEnd) {
		if (!memoryBlockWorkingHeader->used && (memoryBlockWorkingHeader->currSize >= size)) {
			size_t oldSize = memoryBlockWorkingHeader->currSize;
			MemoryBlockHeader* allocatedBlock = memoryBlockWorkingHeader;
			MemoryBlockHeader* afterNewFreeBlock = visitNextBlock(allocatedBlock);
			
			afterNewFreeBlock->prevSize = allocatedBlock->currSize - size;

			allocatedBlock->used = true;
			allocatedBlock->currSize = size;
			MemoryBlockHeader* newFreeBlock = visitNextBlock(allocatedBlock);
			newFreeBlock->currSize = oldSize - size;
			newFreeBlock->used = false;
			newFreeBlock->prevSize = allocatedBlock->currSize;
			return allocatedBlock;
		}
		memoryBlockWorkingHeader = visitNextBlock(memoryBlockWorkingHeader);
	}
	return nullptr;
}

/*
	Resize block to a new size size by address addr. Changes block's address if necessary
*/
void* Allocator::mem_realloc(void* addr, size_t size) {
	if (addr == nullptr)
		return mem_alloc(size);
	normalize(size);
	MemoryBlockHeader* operatedBlock = reinterpret_cast<MemoryBlockHeader*>(addr);
	MemoryBlockHeader* prevBlock = visitPrevBlock(operatedBlock);
	MemoryBlockHeader* nextBlock = visitNextBlock(operatedBlock);
	if (isFree(prevBlock) && isFree(nextBlock) && 
			(prevBlock->currSize + operatedBlock->currSize + nextBlock->currSize >= size)) {
		int oldSize = prevBlock->currSize;
		prevBlock->currSize = size;
		move_data(operatedBlock, prevBlock, operatedBlock->currSize);
		prevBlock->used = true;
		MemoryBlockHeader* afterNewBlock = visitNextBlock(prevBlock);
		if (afterNewBlock == nextBlock) {}
		else {
			afterNewBlock->currSize = oldSize + operatedBlock->currSize + nextBlock->currSize - size;
			afterNewBlock->used = false;
			visitNextBlock(nextBlock)->prevSize = afterNewBlock->currSize;
		}
		afterNewBlock->prevSize = prevBlock->currSize;
		return prevBlock;
	}
	else if (isFree(prevBlock) && prevBlock->currSize + operatedBlock->currSize >= size) {
		int oldSize = prevBlock->currSize;
		prevBlock->currSize = size;
		move_data(operatedBlock, prevBlock, operatedBlock->currSize);
		prevBlock->used = true;
		MemoryBlockHeader* afterNewBlock = visitNextBlock(prevBlock);
		if (afterNewBlock == nextBlock) {}
		else {
			afterNewBlock->currSize = oldSize + operatedBlock->currSize + nextBlock->currSize - size;
			afterNewBlock->used = false;
			nextBlock->prevSize = afterNewBlock->currSize;
		}
		afterNewBlock->prevSize = prevBlock->currSize;
		return prevBlock;
	}
	else if (isFree(nextBlock) && operatedBlock->currSize + nextBlock->currSize >= size) {
		int oldSize = operatedBlock->currSize;
		operatedBlock->currSize = size;
		MemoryBlockHeader* afterNewBlock = visitNextBlock(operatedBlock);
		if (afterNewBlock == visitNextBlock(nextBlock)) {}
		else {
			afterNewBlock->currSize = oldSize + nextBlock->currSize - size;
			afterNewBlock->used = false;
			visitNextBlock(nextBlock)->prevSize = afterNewBlock->currSize;
		}
		afterNewBlock->prevSize = operatedBlock->currSize;
		return operatedBlock;
	}
	else if (operatedBlock->currSize == size)
		return operatedBlock;
	else if (operatedBlock->currSize > size) {
		int oldSize = operatedBlock->currSize;
		operatedBlock->currSize = size;
		nextBlock->prevSize = oldSize - size;
		MemoryBlockHeader* newFreeBlock = visitNextBlock(operatedBlock);
		newFreeBlock->used = false;
		newFreeBlock->prevSize = operatedBlock->currSize;
		newFreeBlock->currSize = oldSize - size;
		return operatedBlock;
	}
	else {
		MemoryBlockHeader* relocatedBlock = reinterpret_cast<MemoryBlockHeader*>(mem_alloc(size));
		if (relocatedBlock != nullptr) {
			move_data(operatedBlock, relocatedBlock, operatedBlock->currSize);
		return relocatedBlock;
		}
	}
}

void Allocator::mem_free(void* addr) {
	MemoryBlockHeader* blockToFree = reinterpret_cast<MemoryBlockHeader*>(addr);
	blockToFree->used = false;
	MemoryBlockHeader* prevBlock = visitPrevBlock(blockToFree);
	MemoryBlockHeader* nextBlock = visitNextBlock(blockToFree);
	bool freePrev = isFree(prevBlock);
	bool freeNext = isFree(nextBlock);
	if (blockToFree == startBlock) {
		startBlock->used = false;
		if (freeNext) {
			mergeTwoBlocks(blockToFree, nextBlock);
		}
	}
	else if (nextBlock == operatedMemoryEnd) {
		if (freePrev) {
			mergeTwoBlocks(prevBlock, blockToFree);
		}
	}
	else {
		if (freePrev && freeNext)
			mergeThreeBlocks(prevBlock, blockToFree, nextBlock);
		else if (freePrev)
			mergeTwoBlocks(prevBlock, blockToFree);
		else if (freeNext)
			mergeTwoBlocks(blockToFree, nextBlock);
	}
}

void Allocator::mergeTwoBlocks(MemoryBlockHeader* header1, MemoryBlockHeader* header2) {
	header1->currSize += header2->currSize;
	MemoryBlockHeader* nextHeader = visitNextBlock(header2);
	nextHeader->prevSize = header1->currSize;
}

void Allocator::mergeThreeBlocks(MemoryBlockHeader* header1, MemoryBlockHeader* header2, MemoryBlockHeader* header3) {
	header1->currSize += header2->currSize + header3->currSize;
	MemoryBlockHeader* nextNextHeader = visitNextBlock(header3);
	nextNextHeader->prevSize = header1->currSize;
}

void Allocator::mem_dump() {
	MemoryBlockHeader* workingBlock = startBlock;
	printf("Begin %p\n", startBlock);
	printf("--------------------------------------------------\n");
	printf("Addr       | next       | prev       | used | size\n");

	while (workingBlock < operatedMemoryEnd) {
		printf("%p   | %p   | %p   | %01d    | %4d\n", workingBlock, 
			visitNextBlock(workingBlock), visitPrevBlock(workingBlock), 
			workingBlock->used, workingBlock->currSize);
		workingBlock = visitNextBlock(workingBlock);
	}
	printf("End of used memory in allocator: %p\n\n\n", operatedMemoryEnd);
}

void Allocator::normalize(size_t& size) {
	if (size < BLOCK_HEADER_SIZE)
		size = BLOCK_HEADER_SIZE;
	while (size % 4 != 0)
		size++;
}