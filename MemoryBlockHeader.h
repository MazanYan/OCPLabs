#include <cstddef>

#ifndef __MemoryBlockHeader_h
#define __MemoryBlockHeader_h

typedef struct {
	bool used;
	size_t currSize;
	size_t prevSize;
} MemoryBlockHeader;

#define BLOCK_HEADER_SIZE sizeof(MemoryBlockHeader)

#endif 