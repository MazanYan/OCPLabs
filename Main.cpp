#include "test.h"
#include "MemoryBlockHeader.h"
#include <stdio.h>

int main(int argc, char** argv) {
	printf("MemoryBlockHeader size: %d bytes\n", BLOCK_HEADER_SIZE);
	testAlloc();

	return 0;
}
