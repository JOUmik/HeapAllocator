#include "BitArray.h"

BitArray::BitArray(size_t i_numBits, bool i_bInitToZero) {
	m_numBytes = i_numBits % bitsPerElement == 0 ? i_numBits / bitsPerElement : i_numBits / bitsPerElement + 1;

	m_pBits = new t_BitData[m_numBytes];
	//memset(m_pBits, i_bInitToZero ? 0 : 1, m_numBytes);
	if (i_bInitToZero) {
		ClearAll();
	}
	else {
		SetAll();
	}
}

BitArray::~BitArray() {
}

void BitArray::Destructor() {
	delete[] m_pBits;
}

void BitArray::ClearAll() {
	for (size_t i = 0; i < m_numBytes; i++) {
		m_pBits[i] = 0;
	}
}

void BitArray::SetAll() {
	for (int i = 0; i < m_numBytes; i++) {
#ifdef _WIN32
		m_pBits[i] = UINT32_MAX;
#else
		m_pBits[i] = UINT64_MAX;
#endif // WIN32
	}
}

void BitArray::SetBit(size_t i_bitNumber) {
	size_t m = i_bitNumber / bitsPerElement;
	size_t n = i_bitNumber % bitsPerElement;

	m_pBits[m] = m_pBits[m] | 1u << n;
}

void BitArray::ClearBit(size_t i_bitNumber) {
	size_t m = i_bitNumber / bitsPerElement;
	size_t n = i_bitNumber % bitsPerElement;

	m_pBits[m] = m_pBits[m] & ~(1u << n);
}

bool BitArray::AreAllBitsClear(void) const{
	for (size_t i = 0; i < m_numBytes; i++) {
		if (m_pBits[i] != t_BitData(0)) return false;
	}

	return true;
}

bool BitArray::AreAllBitsSet(void) const{
	for (size_t i = 0; i < m_numBytes; i++) {
		if (m_pBits[i] == t_BitData(0)) return false;
	}

	return true;
}

bool BitArray::IsBitSet(size_t i_bitNumber) const{
	size_t m = i_bitNumber / bitsPerElement;
	size_t n = i_bitNumber % bitsPerElement;

	if (m_pBits[m] & (1u << n)) {
		return true;
	}

	return false;
}
bool BitArray::IsBitClear(size_t i_bitNumber) const{
	size_t m = i_bitNumber / bitsPerElement;
	size_t n = i_bitNumber % bitsPerElement;

	if (m_pBits[m] & (1u << n)) {
		return false;
	}

	return true;
}

bool BitArray::FindFirstSetBit(size_t& o_firstSetBitIndex){
	size_t index = 0;

	//quick skip bytes where no bits are set
	while ((m_pBits[index] == t_BitData(0)) && (index < m_numBytes)) {
		index++;
	}
	
	//there is no byte has set bits, return false
	if (index == m_numBytes) return false;

	t_BitData Bits = m_pBits[index];
	size_t bit;

	unsigned char isNonzero;
	unsigned long i;
#ifdef _WIN32
	isNonzero = _BitScanForward(&i, Bits);
#else
	isNonzero = _BitScanForward64(&i, Bits);
#endif // _WIN32

	bit = i;

	// now found the byte and the bit
	o_firstSetBitIndex = (index * bitsPerElement) + bit;
	return true;
}

bool BitArray::FindFirstClearBit(size_t& o_firstClearBitIndex){
	size_t index = 0;

	//quick skip bytes where have bits are set
	while ((m_pBits[index] != t_BitData(0)) && (index < m_numBytes)) {
		index++;
	}
	

	if (index == m_numBytes) return false;

	t_BitData Bits = m_pBits[index];
	size_t bit;

	for (bit = 0; bit < bitsPerElement; bit++) {
		if (Bits & (1u << bit)) {
			continue;
		}
		else {
			break;
		}
	}

	
	o_firstClearBitIndex = (index * bitsPerElement) + bit;
	return true;
}