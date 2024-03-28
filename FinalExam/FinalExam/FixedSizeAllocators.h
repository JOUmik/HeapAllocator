#pragma once
#include "BitArray.h"
#include<iostream>

class FixedSizeAllocator {
public:
	FixedSizeAllocator(void* i_baseMemory, size_t i_sizeBlock, size_t i_numsBlock);
	~FixedSizeAllocator();

	void Destructor();

	void* alloc();
	void free(void* i_address);
	bool Contains(void* i_address);

private:
	BitArray* m_pAvailableBlock;
	void* m_baseAllocatorMemory;
	size_t m_BlockSize;
	size_t m_BlockNums;
};