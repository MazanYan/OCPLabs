#include <cstddef>

#ifndef __MemoryBlockHeader_h
#define __MemoryBlockHeader_h

//#pragma pack (1)
typedef struct {
	bool used;
	size_t currSize;
	size_t prevSize;
} MemoryBlockHeader;

#define BLOCK_HEADER_SIZE sizeof(MemoryBlockHeader)

//#pragma pack ()
#endif 