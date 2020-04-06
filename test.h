#include "Allocator.h"
#include <stdio.h>
#include <vector>
#include <random>
#include <numeric>
#include <ctime>

#ifndef __test_h
#define __test_h

void testAlloc() {
	std::srand(unsigned(std::time(0)));
	std::random_device rd;

	Allocator testAlloc = Allocator(1000);

	// allocate 10 random sized blocks
	std::vector<int*> addresses;
	for (int i = 0; i < 10; i++) {
		int random_size_block = rd() % 100 + 50;
		void* address = testAlloc.mem_alloc(random_size_block);
		if (address == nullptr)
			printf("Failed to allocate block with size %d - not enough space!\n", random_size_block);
		else {
			int allocated_size = reinterpret_cast<MemoryBlockHeader*>(address)->currSize;
			addresses.push_back(reinterpret_cast<int*>(address));
			printf("Allocated block with size %3d by address %p normalized size %d\n", random_size_block, address, allocated_size);
		}
	}

	printf("\nAfter allocating %d random sized blocks:\n", addresses.size());
	testAlloc.mem_dump();

	int half_blocks = addresses.size() / 2;
	printf("\nFreeing up %d random blocks\n", half_blocks);

	// set random blocks to free
	std::vector<unsigned int> random_address_index(addresses.size());
	std::iota(random_address_index.begin(), random_address_index.end(), 0);
	std::random_shuffle(random_address_index.begin(), random_address_index.end());

	for (int i = 0; i < half_blocks; i++) {
		void* block_to_free = addresses[random_address_index[i]];
		int old_size = reinterpret_cast<MemoryBlockHeader*>(block_to_free)->currSize;
		testAlloc.mem_free(reinterpret_cast<void*>(block_to_free));
		printf("Freed block by address %p with size %d\n", addresses[i], old_size);
	}
	printf("\nAfter freeing up %d random blocks:\n", half_blocks);
	testAlloc.mem_dump();

	// realloc other blocks
	printf("\n\nReallocating other %d blocks\n", addresses.size() - half_blocks);
	for (int i = half_blocks; i < addresses.size(); i++) {
		int new_size = rd() % 300 + 25;
		int old_size = reinterpret_cast<MemoryBlockHeader*>(addresses[random_address_index[i]])->currSize;
		void* new_address = testAlloc.mem_realloc(addresses[random_address_index[i]], new_size);
		if (new_address == nullptr)
			printf("Failed to realloc by address %p with old size: %d to new size %d\n", addresses[random_address_index[i]], old_size, new_size);
		else {
			int normalized_new_size = reinterpret_cast<MemoryBlockHeader*>(new_address)->currSize;
			printf("Reallocated by address: %p to address: %p old size: %d new size: %d normalized new size: %d\n", addresses[random_address_index[i]], new_address, old_size, new_size, normalized_new_size);
		}
	}
	printf("\nAfter reallocating other %d blocks:\n", addresses.size() - half_blocks);
	testAlloc.mem_dump();
}

#endif