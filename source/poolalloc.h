// =========================================================================== //
//                                                                             //
// Copyright 2009 Maxime Chevalier-Boisvert and McGill University.             //
//                                                                             //
//   Licensed under the Apache License, Version 2.0 (the "License");           //
//   you may not use this file except in compliance with the License.          //
//   You may obtain a copy of the License at                                   //
//                                                                             //
//       http://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                             //
//   Unless required by applicable law or agreed to in writing, software       //
//   distributed under the License is distributed on an "AS IS" BASIS,         //
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  //
//   See the License for the specific language governing permissions and       //
//  limitations under the License.                                             //
//                                                                             //
// =========================================================================== //

// Include guards
#ifndef POOLALLOC_H_
#define POOLALLOC_H_

// Header files
#include <cassert>
#include <iostream>
#include <memory>
#include <limits>
#include <vector>
#include "platform.h"
using std::ptrdiff_t;
typedef std::vector<byte*> PointerVector;

template <class T> class PoolAlloc
{
public:

	// Memory chunk size constant
	static const size_t CHUNK_SIZE = 50000;
	
	// Type definitions
	typedef size_t    size_type;
	typedef ptrdiff_t difference_type;
	typedef T*        pointer;
	typedef const T*  const_pointer;
	typedef T&        reference;
	typedef const T&  const_reference;
	typedef T         value_type;
	
	// Allocator rebinding structure
	template <class U> struct rebind { typedef PoolAlloc<U> other; };
	
	// Constructors and destructor
	PoolAlloc() {}
	PoolAlloc(const PoolAlloc&) {}
	template <class U> PoolAlloc(const PoolAlloc<U>&) {}
	~PoolAlloc() {}
	
	// Methods to get an address from a reference
	pointer address(reference r) const { return &r; }
	const_pointer address(const_reference r) const { return &r; }
	
	// Method to allocate memory
	pointer allocate(size_type cnt, std::allocator<void>::const_pointer = 0)
	{
		// Ensure that the count is 1
		assert (cnt == 1);
		
		// If no free units are available
		if (m_freeUnits.empty())
		{
			// std::cout << "Expanding pool" << std::endl;

			// Allocate a new memory chunk
			byte* pNewChunk = new byte[sizeof(value_type) * CHUNK_SIZE];
			
			// Add the new chunk to the list
			m_chunks.push_back(pNewChunk);
			
			// Reserve room in the free unit vector
			m_freeUnits.reserve(m_freeUnits.capacity() + CHUNK_SIZE);
			
			// Add all free units to the free list
			const byte* pEnd = pNewChunk + sizeof(value_type) * CHUNK_SIZE;
			for (byte* pUnitPtr = pNewChunk; pUnitPtr != pEnd; pUnitPtr += sizeof(value_type))
				m_freeUnits.push_back(pUnitPtr);
		}
		
		// Get the last unit from the free list and remove it
		pointer pUnitPtr = (pointer)m_freeUnits.back();
		m_freeUnits.pop_back();
		
		// Return the unit
		return pUnitPtr;
	}
	
	// Method to deallocate memory
	void deallocate(pointer p, size_type n)
	{ 
		// Add the unit back to the free list
		m_freeUnits.push_back((byte*)p);
    }

	// Method to get the maximum allocatable size
	size_type max_size() const
	{ 
		return std::numeric_limits<size_type>::max() / sizeof(T);
	}
	
	// Method to construct values
	void construct(pointer p, const T& val) { new(p) T(val); }
	
	// Method to destroy values
	void destroy(pointer p) { p->~T(); }

	// Allocator comparison operators
	inline bool operator == (PoolAlloc const&) { return true; }
	inline bool operator != (PoolAlloc const& a) { return !operator==(a); }
	
private:

	// Memory chunk list
	static PointerVector m_chunks;
	
	// Free unit list	
	static PointerVector m_freeUnits;
};

// Memory chunk list
template <class T> PointerVector PoolAlloc<T>::m_chunks;
	
// Free unit list
template <class T> PointerVector PoolAlloc<T>::m_freeUnits;

#endif // #ifndef POOLALLOC_H_
