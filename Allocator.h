#ifndef __Allocator_h
#define __Allocator_h

#include "MemoryBlockHeader.h"

typedef size_t MEMORY_SECTION_SIZE;

class Allocator {

private:
	MEMORY_SECTION_SIZE operatedMemorySize;
	void move_data(void* from_addr, void* to_addr, size_t size);
	MemoryBlockHeader* startBlock;
	MemoryBlockHeader* operatedMemoryEnd;
	void merge(MemoryBlockHeader* block);
	bool isFree(MemoryBlockHeader* block);
	MemoryBlockHeader* visitPrevBlock(MemoryBlockHeader* block);
	MemoryBlockHeader* visitNextBlock(MemoryBlockHeader* block);
	void normalize(size_t& size);
	void mergeTwoBlocks(MemoryBlockHeader* header1, MemoryBlockHeader* header2);
	void mergeThreeBlocks(MemoryBlockHeader* header1, MemoryBlockHeader* header2, MemoryBlockHeader* header3);
public:
	Allocator(size_t size);
	void* mem_alloc(size_t size);
	void* mem_realloc(void* addr, size_t size);
	void mem_free(void* addr);
	void mem_dump();
};

#endif