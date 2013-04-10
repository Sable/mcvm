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
#ifndef CONSTEXPRS_H_
#define CONSTEXPRS_H_

// Header files
#include "expressions.h"
#include "platform.h"
#include "utility.h"

/***************************************************************
* Class   : IntConstExpr
* Purpose : Represent an integer constant expression
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
class IntConstExpr : public Expression
{
public:
	
	// Constructor
	IntConstExpr(int64 value) : m_value(value) { m_exprType = INT_CONST; }
	
	// Method to recursively copy this node
	IntConstExpr* copy() const { return new IntConstExpr(m_value); }
	
	// Method to obtain a string representation of this node
	std::string toString() const { return ::toString(m_value); }
	
	// Accessor to get the internal value
	int64 getValue() const { return m_value; }
	
private:
	
	// Internal value
	int64 m_value;
};

/***************************************************************
* Class   : FPConstExpr
* Purpose : Represent a floating-point constant expression
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
class FPConstExpr : public Expression
{
public:
	
	// Constructor
	FPConstExpr(float64 value) : m_value(value) { m_exprType = FP_CONST; }
	
	// Method to recursively copy this node
	FPConstExpr* copy() const { return new FPConstExpr(m_value); }
	
	// Method to obtain a string representation of this node
	std::string toString() const { return ::toString(m_value); }
	
	// Accessor to get the internal value
	float64 getValue() const { return m_value; }
	
private:
	
	// Internal value
	float64 m_value;
};

/***************************************************************
* Class   : StrConstExpr
* Purpose : Represent a string constant expression
* Initial : Maxime Chevalier-Boisvert on November 8, 2008
****************************************************************
Revisions and bug fixes:
*/
class StrConstExpr : public Expression
{
public:
	
	// Constructor
	StrConstExpr(const std::string& value) : m_value(value) { m_exprType = STR_CONST; }
	
	// Method to recursively copy this node
	StrConstExpr* copy() const { return new StrConstExpr(m_value); }
	
	// Method to obtain a string representation of this node
	std::string toString() const { return "\'" + m_value + "\'"; }
	
	// Accessor to get the internal value
	const std::string& getValue() const { return m_value; }
	
private:
	
	// Internal value
	std::string m_value;
};

#endif // #ifndef CONSTEXPRS_H_ 
