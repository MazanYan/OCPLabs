#include "Allocator.h"
#include <iostream>

int main(int argc, char** argv) {
	size_t testAllocSize = 1000;
	std::cout << BLOCK_HEADER_SIZE << std::endl;
	Allocator testAlloc = Allocator(testAllocSize);
	void* addr1 = testAlloc.mem_alloc(800);
	void* addr2 = testAlloc.mem_alloc(100);
	testAlloc.mem_free(addr1);
	testAlloc.mem_alloc(500);
	testAlloc.mem_realloc(addr2, 200);
	testAlloc.mem_dump();
	return 0;
}