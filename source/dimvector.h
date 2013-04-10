#ifndef _DIMVECTOR_H
#define _DIMVECTOR_H
#include <cstdlib>
#include <gc_cpp.h>
#include <iostream>
class DimVector:public gc{
	public:
	DimVector(unsigned int s=0,size_t val=0);
	DimVector(size_t *,size_t *);	
	DimVector(const DimVector& dv);
	~DimVector();	
	unsigned int m_n;
	size_t *m_ptr;

	unsigned int size() const{return m_n;}
	bool empty() const{if(m_n==0) return true;else return false;}
	size_t& back(){return m_ptr[m_n-1];}
	size_t& front(){return m_ptr[0];}
	const size_t& back() const{return m_ptr[m_n-1];}
	const size_t& front() const{return m_ptr[0];}
	void insert(unsigned int location, size_t val);
	void resize(unsigned int n,size_t p=0);
	void reserve(size_t n);
	void pop_back();
	void push_back(const size_t& x);
	void operator=(const DimVector& dv);

	typedef size_t *iterator;
	typedef size_t const* const_iterator;
	iterator begin(){return m_ptr;}
	iterator end(){return (m_ptr+m_n);}
	const_iterator begin() const{return m_ptr;}
	const_iterator end() const{return (m_ptr+m_n);}

	const size_t& operator[](int index) const{return m_ptr[index];}
	size_t& operator[](int index);
	bool operator!=(const DimVector& dv) const;

};
#endif
