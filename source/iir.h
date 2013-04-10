// =========================================================================== //
//                                                                             //
// Copyright 2008 Maxime Chevalier-Boisvert and McGill University.             //
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
#ifndef IIR_H_
#define IIR_H_

// Header files
#include <string>
#include <gc_cpp.h>
#include <gc/gc_allocator.h>

/***************************************************************
* Class   : IIRNode
* Purpose : Base class for IIR node objects
* Initial : Maxime Chevalier-Boisvert on October 16, 2008
* Notes   : All IIR nodes are garbage-collected
****************************************************************
Revisions and bug fixes:
*/
class IIRNode : public gc
{
public:
	
	// Enumerate IIR node types
	enum Type
	{
		FUNCTION,
		SEQUENCE,
		STATEMENT,
		EXPRESSION
	};
	
	// Constructor and destructor
	IIRNode() {}
	virtual ~IIRNode() {}
	
	// Method to recursively copy this node
	virtual IIRNode* copy() const = 0;
	
	// Method to obtain a string representation of this node
	virtual std::string toString() const = 0;
	
	// Method to get the type of this IIR node
	Type getType() const { return m_type; }
	
protected:
	
	// Type of this node
	Type m_type;	
};

#endif // #ifndef IIR_H_ 
