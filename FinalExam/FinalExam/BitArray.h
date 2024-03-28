#pragma once
#include <stdint.h>
#include <string.h>
#include <intrin.h>

class BitArray {
#ifdef _WIN32
	typedef uint32_t t_BitData;
#else
	typedef uint64_t t_BitData;
#endif

public:
	BitArray(size_t i_numBits, bool i_bInitToZero);
	~BitArray();

	void Destructor();

	void ClearAll(void);
	void SetAll(void);

	void SetBit(size_t i_bitNumber);
	void ClearBit(size_t i_bitNumber);

	bool AreAllBitsClear(void) const;
	bool AreAllBitsSet(void) const;

	bool IsBitSet(size_t i_bitNumber) const;
	bool IsBitClear(size_t i_bitNumber) const;

	//quickly find the first set or clear bit
	bool FindFirstSetBit(size_t& o_firstSetBitIndex);
	bool FindFirstClearBit(size_t& o_firstClearBitIndex);

	//indexing operator
	//bool operator[](size_t i_bitIndex);
private:
	t_BitData* m_pBits;
	size_t m_numBytes;

	const size_t bitsPerElement = sizeof(t_BitData) * 8;
};

