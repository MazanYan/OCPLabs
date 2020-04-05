#include "Allocator.h"
#include "MemoryBlockHeader.h"
#include <iostream>
#include <cstdio>
using namespace std;

MemoryBlockHeader* Allocator::visitNextBlock(MemoryBlockHeader* block) {
	MemoryBlockHeader* nextBlock = (MemoryBlockHeader*)((int*)block+ block->currSize);
	if (nextBlock > operatedMemoryEnd)
		return operatedMemoryEnd;
	//if (block == operatedMemoryEnd || nextBlock >= operatedMemoryEnd)
	//	return nullptr;
	return nextBlock;
}

MemoryBlockHeader* Allocator::visitPrevBlock(MemoryBlockHeader* block) {
	MemoryBlockHeader* prevBlock = (MemoryBlockHeader*)((int*)block - block->prevSize);
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
	startBlock = (MemoryBlockHeader*) (&memorySpace[0]);
	startBlock->currSize = size;
	startBlock->used = false;
	startBlock->prevSize = 0;
	operatedMemoryEnd = (MemoryBlockHeader*) &memorySpace[size - 1];
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
	if (isFree(nextBlock)) {
		prevBlock->currSize += nextBlock->currSize;

	}
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
		if (!memoryBlockWorkingHeader->used && (memoryBlockWorkingHeader->currSize/* + BLOCK_HEADER_SIZE*/ >= size)) {
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

			mem_dump();
			std::cout << endl << endl << endl;

			return allocatedBlock;
		}
		memoryBlockWorkingHeader = visitNextBlock(memoryBlockWorkingHeader);
	}
	return nullptr;
}

/** Proposed algorithm:
 * void *mem_realloc(void *ptr, size_t size);
1. Если ссылка равна NULL – вызов функции mem_alloc и возврат возвращенной функцией
ссылки.
2. Выравнивание размера до 4 байт.
3. Если два соседних блока свободны и суммарный размер трех блоков превышает или равен
нужному – изменение заголовка предыдущего блока и ,если суммарный размер трех блоков
превышает нужный размер, создание заголовка свободного блока. Изменение заголовка
следующего после следующего блока, если такой есть (размер предыдущего блока). Возврат
ссылки на предыдущий блок.
4. Если предыдущий блок свободен и суммарный размер двух блоков превышает или равен
нужному – изменение заголовка предыдущего блока и ,если суммарный размер двух блоков
превышает нужный размер, создание заголовка свободного блока. Изменение заголовка 
следующего блока, если такой есть (размер предыдущего блока). Возврат ссылки на
предыдущий блок.
5. Если следующий блок свободен и суммарный размер двух блоков превышает или равен
нужному – изменение заголовка текущего блока и ,если суммарный размер двух блоков
превышает нужный размер, создание заголовка свободного блока. Изменение заголовка
следующего после следующего блока, если такой есть (размер предыдущего блока). Возврат
ссылки на текущий блок.
6. Если два соседних блока заняты и размер текущего блока равен нужному - возврат ссылки на
текущий блок.
7. Если два соседних блока заняты и размер текущего блока меньше нужного – изменение
размера текущего блока, создание нового заголовка свободного блока. Изменение заголовка
следующего блока (размер предыдущего блока). Возврат ссылки на текущий блок.


8. Иначе вызов функции mem_alloc. Если возвращенная функцией ссылка не равна NULL,
копирование данных из старого блока в новый. Возврат ссылки на новый блок.
 * */
void* Allocator::mem_realloc(void* addr, size_t size) {
	if (addr == nullptr)
		return mem_alloc(size);
	normalize(size);
	MemoryBlockHeader* operatedBlock = (MemoryBlockHeader*) addr;
	MemoryBlockHeader* prevBlock = visitPrevBlock(operatedBlock);
	MemoryBlockHeader* nextBlock = visitNextBlock(operatedBlock);
	if (isFree(prevBlock) && isFree(nextBlock) && 
			(prevBlock->currSize + operatedBlock->currSize + nextBlock->currSize >= size)) {
		//mergeThreeBlocks(prevBlock, operatedBlock, nextBlock);
		prevBlock->currSize = size;
		move_data(operatedBlock, prevBlock, operatedBlock->currSize);
		prevBlock->used = true;
		MemoryBlockHeader* afterNewBlock = visitNextBlock(prevBlock);
		if (afterNewBlock == nextBlock) {}
		else {
			afterNewBlock->currSize = prevBlock->currSize + operatedBlock->currSize + nextBlock->currSize - size;
			afterNewBlock->used = false;
			nextBlock->prevSize = afterNewBlock->currSize;
		}
		afterNewBlock->prevSize = prevBlock->currSize;
		return prevBlock;
	}
	else if (isFree(prevBlock) && prevBlock->currSize + operatedBlock->currSize >= size) {
		int oldSize = prevBlock->currSize;
		//mergeTwoBlocks(prevBlock, operatedBlock);
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
		//mergeTwoBlocks(operatedBlock, nextBlock);
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
	else if (operatedBlock->currSize < size) {
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
		MemoryBlockHeader* relocatedBlock = (MemoryBlockHeader*) mem_alloc(size);
		if (relocatedBlock != nullptr) {
			move_data(operatedBlock, relocatedBlock, operatedBlock->currSize);
		return relocatedBlock;
		}
	}
}

void Allocator::mem_free(void* addr) {
	MemoryBlockHeader* blockToFree = (MemoryBlockHeader*)addr;
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
	mem_dump();
	std::cout << endl << endl << endl;
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
	MemoryBlockHeader* workingBlock = (MemoryBlockHeader*)startBlock;
	printf("Begin %p\n", startBlock);
	printf("--------------------------------------------------------------------\n");
	printf("Addr             | next             | prev             | used | size\n");

	while (workingBlock < operatedMemoryEnd) {
		printf("%p   | %p   | %p   | %01d    | %4d (%4d)\n", workingBlock, 
			visitNextBlock(workingBlock), visitPrevBlock(workingBlock), 
			workingBlock->used, workingBlock->currSize, workingBlock->currSize - BLOCK_HEADER_SIZE);
		workingBlock = visitNextBlock(workingBlock);
	}
	std::cout << "End of used memory in allocator: " << std::hex << operatedMemoryEnd << endl;
}

void Allocator::normalize(size_t& size) {
	if (size < BLOCK_HEADER_SIZE)
		size = BLOCK_HEADER_SIZE;
	while (size % 4 != 0)
		size++;
}