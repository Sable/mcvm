#include "dimvector.h"
#include <iostream>
#include <cassert>
using namespace std;

static void printDV(const DimVector& dv){
	cout<<"[";
	for(unsigned int i=0;i<dv.size();i++){
		cout<<dv[i]<<" ";
	}
	cout<<"]"<<endl;
}

size_t& DimVector::operator[](int index){
	//cout<<"Setting index "<<index<<" "<<m_n<<endl;
	//printDV(*(this));
	return m_ptr[index];
}

void DimVector::operator=(const DimVector& dv){
	if(this==(&dv)) return;
	if(m_n>0) delete[] m_ptr;
	if(dv.m_n>0){
		m_ptr = new size_t[dv.m_n];
		for(unsigned int i=0;i<dv.m_n;i++){
			m_ptr[i] = dv[i];
		}
	}else{
		m_ptr = NULL;
	}
	m_n = dv.m_n;

}

DimVector::DimVector(unsigned int s,size_t val){
	//cout<<"Constructor 1 with "<<s<<" "<<val<<endl;
	if(s<=0){
		m_n=0;
		m_ptr = NULL;
	}else{
		m_n = s;
		m_ptr = new size_t[s];
	}
	for(unsigned int i=0;i<s;i++){
		m_ptr[i] = val;
	}
}

DimVector::DimVector(size_t *ptr1,size_t *ptr2){
	//cout<<"Constructor 2 with "<<(ptr2-ptr1)<<endl;
	if(ptr1>=ptr2){
		m_n = 0;
		m_ptr = NULL;
	}else{
		ptrdiff_t diff = ptr2-ptr1;
		m_n = (unsigned int)diff;
		//cout<<"Trying to allocate "<<m_n<<" elements in DimVector constructor 2"<<endl;
		m_ptr = new size_t[m_n];
		for(unsigned int i=0;i<m_n;i++){
			m_ptr[i] = ptr1[i];
		}
	}

}

DimVector::DimVector(const DimVector& dv){
	//cout<<"Called copy constructor"<<endl;
	m_n = dv.m_n;
	if(m_n>0){
		m_ptr = new (GC) size_t[m_n];
		for(unsigned int i=0;i<m_n;i++){
			m_ptr[i] = dv[i];
		}
	}else{
		m_ptr = NULL;
	}
}

DimVector::~DimVector(){
	//cout<<"Called destructor"<<endl;
	if(m_n>0) delete[] m_ptr;
}
void DimVector::insert(unsigned int location,size_t val){
	//cout<<"Called insert with "<<location<<" "<<val<<" "<<m_n<<endl;
	if(location>m_n) return;
	size_t *temp_ptr = new size_t[m_n+1];
	for(unsigned int i=0;i<location;i++){
		temp_ptr[i] = m_ptr[i];
	}
	temp_ptr[location] = val;
	for(unsigned int i=location+1;i<m_n+1;i++){
		temp_ptr[i] = m_ptr[i-1];
	}
	if(m_n>0) delete[] m_ptr;
	m_ptr = temp_ptr;
	m_n++;
}

void DimVector::resize(unsigned int n,size_t val){
	//cout<<"Called resize with "<<n<<" "<<val<<endl;
	//printDV(*this);
	if(n==m_n) return;

	if(n==0){
		delete[] m_ptr;
		m_n = 0;
		return;
	}

	size_t *temp_ptr = new (GC) size_t[n];
	if(n>m_n){
		for(unsigned int i=0;i<m_n;i++){
			temp_ptr[i] = m_ptr[i];
		}
		for(unsigned int i=m_n;i<n;i++){
			temp_ptr[i] = val;
		}
	}else{
		for(unsigned int i=0;i<n;i++){
			temp_ptr[i] = m_ptr[i];
		}		
	}
	delete[] m_ptr;
	m_ptr = temp_ptr;
	m_n = n;
	//printDV(*this);
}

void DimVector::push_back(const size_t& val){
	//cout<<"Called push_back "<<val<<endl;
	size_t tempval = val;
	resize(m_n+1,tempval);
}	

void DimVector::pop_back(){
	//cout<<"Called pop_back"<<endl;
	resize(m_n-1);
}

bool DimVector::operator!=(const DimVector& dv) const{
	//cout<<"Called operator !="<<endl;
	if(this->m_n!=dv.m_n) return true;
	for(unsigned int i=0;i<m_n;i++){
		if(this->m_ptr[i]!=dv.m_ptr[i]) return true;
	}
	return false;
}

void DimVector::reserve(size_t n){
	//cout<<"Called reserve "<<n<<endl;
}
