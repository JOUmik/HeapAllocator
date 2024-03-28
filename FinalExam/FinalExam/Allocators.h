#pragma once
#include <inttypes.h>
#include <malloc.h>

#include <stdio.h>
#include "HeapManagerProxy.h"

namespace Allocate {
	void* malloc(size_t i_size)
	{
		void* pReturn = nullptr;
		FixedSizeAllocator* pFSA = HeapManagerProxy::FindFixedSizeAllocator(i_size);

		if (pFSA) {
			pReturn = pFSA->alloc();
		}

		//if no FSA available or there was one and it is full
		if (pReturn == nullptr) {
			pReturn = HeapManagerProxy::alloc(HeapManagerProxy::pHeapManager, i_size);
		}
		return pReturn;
	}

	void free(void* i_ptr)
	{
		if (i_ptr == nullptr) {
			return;
		}

		for (size_t i = 0; i < 3; i++) {
			if (HeapManagerProxy::FSAs[i]->Contains(i_ptr)) {
				HeapManagerProxy::FSAs[i]->free(i_ptr);
				return;
			}
		}

		HeapManagerProxy::free(HeapManagerProxy::pHeapManager, i_ptr);
	}
}

void* operator new[](size_t i_size)
{
	void* pReturn = nullptr;

	FixedSizeAllocator* pFSA = HeapManagerProxy::FindFixedSizeAllocator(i_size);

	if (pFSA) {
		pReturn = pFSA->alloc();
	}

	//if no FSA available or there was one and it is full
	if (pReturn == nullptr) {
		pReturn = HeapManagerProxy::alloc(HeapManagerProxy::pHeapManager, i_size);
	}
	return pReturn;
}

void operator delete [](void* i_ptr)
{
	for (size_t i = 0; i < 3; i++) {
		if (HeapManagerProxy::FSAs[i]->Contains(i_ptr)) {
			HeapManagerProxy::FSAs[i]->free(i_ptr);
			return;
		}
	}


	HeapManagerProxy::free(HeapManagerProxy::pHeapManager, i_ptr);
}