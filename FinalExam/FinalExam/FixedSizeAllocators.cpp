#include "FixedSizeAllocators.h"


FixedSizeAllocator::FixedSizeAllocator(void* i_baseMemory, size_t i_sizeBlock, size_t i_numsBlock) {
	m_baseAllocatorMemory = i_baseMemory;
	m_BlockSize = i_sizeBlock;
	m_BlockNums = i_numsBlock;
	m_pAvailableBlock = new BitArray(i_numsBlock, false);
}

FixedSizeAllocator::~FixedSizeAllocator() {
}

void FixedSizeAllocator::Destructor() {
	if (!m_pAvailableBlock->AreAllBitsSet()) {
		//there are outstanding allocations
#ifdef _DEBUG
		std::cout << "detect outstanding allocations when attempt to destruct" << std::endl;
#endif // _DEBUG
	}

	m_pAvailableBlock->Destructor();
	delete m_pAvailableBlock;
}

void* FixedSizeAllocator::alloc() {
	size_t i_firstAvailable;

	if (m_pAvailableBlock->FindFirstSetBit(i_firstAvailable)) {
		//mark it in  use because we're going to allocate it to user
		m_pAvailableBlock->ClearBit(i_firstAvailable);

		//calculate it's address and return it to user
		return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_baseAllocatorMemory) + (i_firstAvailable * m_BlockSize));
	}
	else {
		return nullptr;
	}
}

void FixedSizeAllocator::free(void* i_address) {
	size_t index = (reinterpret_cast<uintptr_t>(i_address) - reinterpret_cast<uintptr_t>(m_baseAllocatorMemory)) / m_BlockSize;
	size_t remainder = (reinterpret_cast<uintptr_t>(i_address) - reinterpret_cast<uintptr_t>(m_baseAllocatorMemory)) % m_BlockSize;
	if (remainder != 0) {
#ifdef _DEBUG
		std::cout << "FixedSizeAllocator(free): Address remainder is not 0!" << std::endl;
#endif // _DEBUG
	}

	if (!m_pAvailableBlock->IsBitClear(index)) {
#ifdef _DEBUG
		std::cout << "FixedSizeAllocator(free): Address has been freed before!" << std::endl;
#endif // _DEBUG
	}
	else {
		m_pAvailableBlock->SetBit(index);
	}
}

bool FixedSizeAllocator::Contains(void* i_address) {
	if (i_address >= m_baseAllocatorMemory && i_address < reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_baseAllocatorMemory) + (m_BlockNums * m_BlockSize))) {
		return true;
	}
	else {
		return false;
	}
}