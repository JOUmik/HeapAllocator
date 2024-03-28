#pragma once
#include <assert.h>
#include <vector>
#include "FixedSizeAllocators.h"

namespace HeapManagerProxy {
	struct MemoryBlock {
		void* pBaseAddress;
		size_t BlockSize;
		struct MemoryBlock* pNextBlock;
	};

	struct FSAInitData
	{
		size_t sizeBlock;
		size_t numBlocks;
	};

	class HeapManager {
	public:
		HeapManager();

		struct MemoryBlock* theHeap;
		struct MemoryBlock* freeList;
		struct MemoryBlock* pFreeList = nullptr;
		struct MemoryBlock* outstandingAllocations = nullptr;
	};

	HeapManager* CreateHeapManager(void* pHeapMemory, size_t sizeHeap, unsigned int numDescriptors);
	void InitializeMemoryBlocks(HeapManager* i_pHeapManager, void* i_pBlocksMemory, size_t i_BlocksMemoryBytes);
	void* alloc(HeapManager* i_pHeapManager, size_t i_size, unsigned int alignment = 4);
	bool free(HeapManager* i_pHeapManager, void* i_ptr);
	MemoryBlock* FindFirstFittingFreeBlock(HeapManager* i_pHeapManager, const size_t i_size, unsigned int alignment);
	MemoryBlock* GetFreeMemoryBlock(HeapManager* i_pHeapManager);
	void ReturnMemoryBlock(HeapManager* i_pHeapManager, MemoryBlock* i_pFreeBlock);
	void RemoveFromFreeList(HeapManager* i_pHeapManager, MemoryBlock* _pBlock);
	void TrackAllocation(HeapManager* i_pHeapManager, MemoryBlock* _pBlock);
	MemoryBlock* RemoveOutstandingAllocation(HeapManager* i_pHeapManager, const void* _i_ptr);
	void Collect();
	bool Contains(HeapManager* i_pHeapManager, const void* _i_ptr);
	bool IsAllocated(HeapManager* i_pHeapManager, const void* _i_ptr);
	size_t AlignUp(size_t i_value, const unsigned int i_align);
	void Destroy(HeapManager* i_pHeapManager);


	FixedSizeAllocator* FindFixedSizeAllocator(size_t i_size);

	HeapManager* pHeapManager;
	FSAInitData FSASizes[3] = { {16, 100}, {32, 200}, {96, 400} };
	FixedSizeAllocator* FSAs[3];
	void* FSABaseAddresses[3];

	HeapManager* CreateHeapManager(void* pHeapMemory, size_t sizeHeap, unsigned int numDescriptors) {
		size_t sizeForAllocate = sizeHeap - numDescriptors * sizeof(MemoryBlock)
			- FSASizes[0].numBlocks * FSASizes[0].sizeBlock
			- FSASizes[1].numBlocks * FSASizes[1].sizeBlock
			- FSASizes[2].numBlocks * FSASizes[2].sizeBlock;

		pHeapManager = reinterpret_cast<HeapManager*>(pHeapMemory);

		pHeapMemory = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pHeapMemory) + sizeof(HeapManager));

		void* heapBaseAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pHeapMemory) + numDescriptors * sizeof(MemoryBlock)
			+ FSASizes[0].numBlocks * FSASizes[0].sizeBlock
			+ FSASizes[1].numBlocks * FSASizes[1].sizeBlock
			+ FSASizes[2].numBlocks * FSASizes[2].sizeBlock);

		FSABaseAddresses[0] = (reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pHeapMemory) + numDescriptors * sizeof(MemoryBlock)));

		FSABaseAddresses[1] = (reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pHeapMemory) + numDescriptors * sizeof(MemoryBlock)
			+ FSASizes[0].numBlocks * FSASizes[0].sizeBlock));

		FSABaseAddresses[2] = (reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pHeapMemory) + numDescriptors * sizeof(MemoryBlock)
			+ FSASizes[0].numBlocks * FSASizes[0].sizeBlock
			+ FSASizes[1].numBlocks * FSASizes[1].sizeBlock));

		InitializeMemoryBlocks(pHeapManager, pHeapMemory, numDescriptors * sizeof(MemoryBlock));

		pHeapManager->theHeap = GetFreeMemoryBlock(pHeapManager);

		pHeapManager->theHeap->pBaseAddress = heapBaseAddress;
		pHeapManager->theHeap->BlockSize = sizeForAllocate;
		pHeapManager->theHeap->pNextBlock = nullptr;

		pHeapManager->freeList = pHeapManager->theHeap;


		for (int i = 0; i < 3; i++) {
			FSAs[i] = (new FixedSizeAllocator(FSABaseAddresses[i], FSASizes[i].sizeBlock, FSASizes[i].numBlocks));
		}

		return pHeapManager;
	}

	void InitializeMemoryBlocks(HeapManager* i_pHeapManager, void* i_pBlocksMemory, size_t i_BlocksMemoryBytes) {
		assert((i_pBlocksMemory != nullptr) && (i_BlocksMemoryBytes > sizeof(MemoryBlock)));

		i_pHeapManager->pFreeList = reinterpret_cast<MemoryBlock*>(i_pBlocksMemory);
		const size_t NumberofBlocks = i_BlocksMemoryBytes / sizeof(MemoryBlock);

		MemoryBlock* pCurrentBlock = i_pHeapManager->pFreeList;

		for (size_t i = 0; i < NumberofBlocks; i++) {
			//std::cout << pCurrentBlock << std::endl;

			pCurrentBlock->pBaseAddress = nullptr;
			pCurrentBlock->BlockSize = 0;
			pCurrentBlock->pNextBlock = reinterpret_cast<MemoryBlock*>(reinterpret_cast<uintptr_t>(pCurrentBlock) + sizeof(MemoryBlock));
			pCurrentBlock = pCurrentBlock->pNextBlock;
		}
		//last block, end the list
		pCurrentBlock->pBaseAddress = nullptr;
		pCurrentBlock->BlockSize = 0;
		pCurrentBlock->pNextBlock = nullptr;
	}

	void* alloc(HeapManager* i_pHeapManager, size_t i_size, unsigned int alignment) {
		//track this allocation
		MemoryBlock* pBlock = GetFreeMemoryBlock(i_pHeapManager);
		if (pBlock == nullptr) {
			return nullptr;
		}

		i_size = AlignUp(i_size, alignment);

		MemoryBlock* pFreeBlock = FindFirstFittingFreeBlock(i_pHeapManager, i_size, alignment);

		//oh no. we didn't find a block big enough
		if (pFreeBlock == nullptr) {
			return nullptr;
		}

		//pFreeBlock.pBaseAddress is what we return to user
		if (pFreeBlock->BlockSize == i_size) {
			RemoveFromFreeList(i_pHeapManager, pFreeBlock);
			TrackAllocation(i_pHeapManager, pFreeBlock);

			return pFreeBlock->pBaseAddress;
		}
		else {
			pBlock->pBaseAddress = pFreeBlock->pBaseAddress;
			pBlock->BlockSize = i_size;


			TrackAllocation(i_pHeapManager, pBlock);

			//adjust pBaseAddress past user bytes and reduce BlockSize by number of bytes provided to user
			uintptr_t sizePtr = reinterpret_cast<uintptr_t>(pFreeBlock->pBaseAddress);
			sizePtr += i_size;

			pFreeBlock->pBaseAddress = reinterpret_cast<void*>(sizePtr);

			pFreeBlock->BlockSize -= i_size;

			return pBlock->pBaseAddress;
		}
	}

	bool free(HeapManager* i_pHeapManager, void* i_ptr) {
		//remove the block for this pointer from outstandingAllocations
		MemoryBlock* pBlock = RemoveOutstandingAllocation(i_pHeapManager, i_ptr);
		assert(pBlock);

		//put the block on freeList and coalesce the blocks when two or three blocks abut with each other.
		MemoryBlock* pFreeBlock = i_pHeapManager->freeList;
		MemoryBlock* pFreeNextBlock = pFreeBlock->pNextBlock;

		//if the address is smaller than any block of the freeList
		if (pFreeBlock->pBaseAddress > i_ptr) {
			if (reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pBlock->pBaseAddress) + pBlock->BlockSize) == pFreeBlock->pBaseAddress) {
				pFreeBlock->BlockSize += pBlock->BlockSize;
				pFreeBlock->pBaseAddress = i_ptr;

				ReturnMemoryBlock(i_pHeapManager, pBlock);
			}
			else {
				pBlock->pNextBlock = pFreeBlock;
				i_pHeapManager->freeList = pBlock;
			}
		}
		else {
			while (pFreeNextBlock != nullptr) {
				//Find the right place
				if (pFreeBlock->pBaseAddress < i_ptr && pFreeNextBlock->pBaseAddress > i_ptr) {
					//if the block freed can coalesce with the left block
					if (reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pFreeBlock->pBaseAddress) + pFreeBlock->BlockSize) == pBlock->pBaseAddress) {
						pFreeBlock->BlockSize += pBlock->BlockSize;
						//if the block freed also can coalesce with the right block
						if (reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pFreeBlock->pBaseAddress) + pFreeBlock->BlockSize) == pFreeNextBlock->pBaseAddress) {
							pFreeBlock->BlockSize += pFreeNextBlock->BlockSize;
							MemoryBlock* n_pFreeNextBlock = pFreeNextBlock->pNextBlock;
							pFreeBlock->pNextBlock = n_pFreeNextBlock;
							ReturnMemoryBlock(i_pHeapManager, pFreeNextBlock);

						}
						ReturnMemoryBlock(i_pHeapManager, pBlock);
					}
					//if the block freed can coalesce with the right block
					else if (reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pBlock->pBaseAddress) + pBlock->BlockSize) == pFreeNextBlock->pBaseAddress) {
						pFreeNextBlock->BlockSize += pBlock->BlockSize;
						pFreeNextBlock->pBaseAddress = pBlock->pBaseAddress;

						ReturnMemoryBlock(i_pHeapManager, pBlock);
					}
					//cannot coalesce, put the block between the left and right blocks
					else {
						pFreeBlock->pNextBlock = pBlock;
						pBlock->pNextBlock = pFreeNextBlock;
					}

					break;
				}
				pFreeBlock = pFreeBlock->pNextBlock;
				pFreeNextBlock = pFreeNextBlock->pNextBlock;
			}

			//if the address is bigger than any block of the freeList
			if (pFreeNextBlock == nullptr) {
				if (reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pFreeBlock->pBaseAddress) + pFreeBlock->BlockSize) == pBlock->pBaseAddress) {
					pFreeBlock->BlockSize += pBlock->BlockSize;

					ReturnMemoryBlock(i_pHeapManager, pBlock);
				}
				else {
					pFreeBlock->pNextBlock = pBlock;
					pBlock->pNextBlock = nullptr;
				}
			}
		}

		return true;
	}

	MemoryBlock* FindFirstFittingFreeBlock(HeapManager* i_pHeapManager, const size_t i_size, unsigned int alignment) {
		MemoryBlock* pFreeBlock = i_pHeapManager->freeList;
		while (pFreeBlock) {
			if ((reinterpret_cast<uintptr_t>(pFreeBlock->pBaseAddress) & (alignment - 1)) == 0) {

				if (pFreeBlock->BlockSize >= i_size) {
					break;
				}
			}
			else {
				size_t targetSize = i_size + alignment - (reinterpret_cast<uintptr_t>(pFreeBlock->pBaseAddress) & (alignment - 1));

				if (pFreeBlock->BlockSize >= targetSize) {
					MemoryBlock* newBlock = GetFreeMemoryBlock(i_pHeapManager);

					if (newBlock == nullptr)
					{
						return newBlock;
					}

					newBlock->pBaseAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pFreeBlock->pBaseAddress) + alignment - (reinterpret_cast<uintptr_t>(pFreeBlock->pBaseAddress) & (alignment - 1)));
					newBlock->BlockSize = pFreeBlock->BlockSize - (alignment - (reinterpret_cast<uintptr_t>(pFreeBlock->pBaseAddress) & (alignment - 1)));

					pFreeBlock->BlockSize = alignment - (reinterpret_cast<uintptr_t>(pFreeBlock->pBaseAddress) & (alignment - 1));

					newBlock->pNextBlock = pFreeBlock->pNextBlock;

					pFreeBlock->pNextBlock = newBlock;

					pFreeBlock = pFreeBlock->pNextBlock;


					break;
				}
			}

			pFreeBlock = pFreeBlock->pNextBlock;
		}

		return pFreeBlock;
	}

	MemoryBlock* GetFreeMemoryBlock(HeapManager* i_pHeapManager) {
		//assert(i_pHeapManager->pFreeList != nullptr);
		if (i_pHeapManager->pFreeList == nullptr) {
			return nullptr;
		}

		MemoryBlock* pReturnBlock = i_pHeapManager->pFreeList;
		i_pHeapManager->pFreeList = i_pHeapManager->pFreeList->pNextBlock;

		return pReturnBlock;
	};

	void ReturnMemoryBlock(HeapManager* i_pHeapManager, MemoryBlock* i_pFreeBlock) {
		assert(i_pFreeBlock != nullptr);

		i_pFreeBlock->pBaseAddress = nullptr;
		i_pFreeBlock->BlockSize = 0;
		i_pFreeBlock->pNextBlock = i_pHeapManager->pFreeList;

		i_pHeapManager->pFreeList = i_pFreeBlock;
	}

	void RemoveFromFreeList(HeapManager* i_pHeapManager, MemoryBlock* _pBlock) {
		MemoryBlock* pFreeBlock = i_pHeapManager->freeList;
		MemoryBlock* pFreeNextBlock = pFreeBlock->pNextBlock;

		if (pFreeBlock == _pBlock) {
			i_pHeapManager->freeList = i_pHeapManager->freeList->pNextBlock;
		}
		else {
			while (pFreeNextBlock != _pBlock) {
				pFreeBlock = pFreeNextBlock;
				pFreeNextBlock = pFreeNextBlock->pNextBlock;
			}

			pFreeBlock->pNextBlock = pFreeNextBlock->pNextBlock;
		}

	}

	void TrackAllocation(HeapManager* i_pHeapManager, MemoryBlock* _pBlock) {
		//put the block on outstandingAllocations

		_pBlock->pNextBlock = i_pHeapManager->outstandingAllocations;
		i_pHeapManager->outstandingAllocations = _pBlock;
	};

	MemoryBlock* RemoveOutstandingAllocation(HeapManager* i_pHeapManager, const void* _i_ptr) {
		MemoryBlock* pOutstandingBlock = i_pHeapManager->outstandingAllocations;
		MemoryBlock* pOutstandingNextBlock = pOutstandingBlock->pNextBlock;

		MemoryBlock* pReturnBlock;

		if (pOutstandingBlock->pBaseAddress == _i_ptr) {
			i_pHeapManager->outstandingAllocations = i_pHeapManager->outstandingAllocations->pNextBlock;

			pReturnBlock = pOutstandingBlock;
		}
		else {
			while (pOutstandingNextBlock->pBaseAddress != _i_ptr) {
				pOutstandingBlock = pOutstandingBlock->pNextBlock;
				pOutstandingNextBlock = pOutstandingNextBlock->pNextBlock;
			}

			pReturnBlock = pOutstandingNextBlock;
			pOutstandingBlock->pNextBlock = pOutstandingNextBlock->pNextBlock;
		}


		return pReturnBlock;
	}

	void Collect() {
		//I have done the merge job on free()
	}

	bool Contains(HeapManager* i_pHeapManager, const void* _i_ptr) {
		return true;
	}

	bool IsAllocated(HeapManager* i_pHeapManager, const void* _i_ptr) {
		MemoryBlock* pOutstandingBlock = i_pHeapManager->outstandingAllocations;

		while (pOutstandingBlock != nullptr) {
			if (pOutstandingBlock->pBaseAddress == _i_ptr) {
				return true;
			}

			pOutstandingBlock = pOutstandingBlock->pNextBlock;
		}

		return false;
	}

	size_t AlignUp(size_t i_value, const unsigned int i_align) {
		if (i_value % i_align == 0) {
			return i_value;
		}

		i_value = i_value + i_align - (i_value % i_align);


		return i_value;
	}

	void Destroy(HeapManager* i_pHeapManager) {
		i_pHeapManager = nullptr;

		for (int i = 0; i < 3; i++) {
			FSAs[i]->Destructor();
			delete FSAs[i];
		}
	}

	FixedSizeAllocator* FindFixedSizeAllocator(size_t i_size) {
		if (i_size <= FSASizes[0].sizeBlock) {
			return FSAs[0];
		}
		else if (i_size <= FSASizes[1].sizeBlock) {
			return FSAs[1];
		}
		else if (i_size <= FSASizes[2].sizeBlock) {
			return FSAs[2];
		}

		return nullptr;
	}
}