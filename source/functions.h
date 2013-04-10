// =========================================================================== //
//                                                                             //
// Copyright 2008-2009 Maxime Chevalier-Boisvert and McGill University.        //
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
#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

// Header files
#include "iir.h"
#include "objects.h"
#include "environment.h"
#include "stmtsequence.h"
#include "symbolexpr.h"
#include "environment.h"
#include "arrayobj.h"
#include "typeinfer.h"

// Temporary variable name prefix
const std::string TEMP_VAR_PREFIX = "$t";

/***************************************************************
* Class   : Function
* Purpose : Base class for functions
* Initial : Maxime Chevalier-Boisvert on October 27, 2008
****************************************************************
Revisions and bug fixes:
*/
class Function : public IIRNode, DataObject
{
public:
	
	// Constructor and destructor
	Function() { IIRNode::m_type = IIRNode::FUNCTION; DataObject::m_type = DataObject::FUNCTION; }
	virtual ~Function() {}
		
	// Method to recursively copy this node
	virtual Function* copy() const = 0;
	
	// Mutator to set the function name
	void setFuncName(const std::string& name) { m_funcName = name; }
	
	// Accessor to get the function name
	const std::string& getFuncName() const { return m_funcName; }
	
	// Accessor to tell if this is a program function
	bool isProgFunction() const { return m_isProgFunction; }
	
protected:
	
	// Function name
	std::string m_funcName;
	
	// Variable to indicate whether this is a program function
	bool m_isProgFunction;
};

/***************************************************************
* Class   : LibFunction
* Purpose : Implement a library function interface
* Initial : Maxime Chevalier-Boisvert on October 29, 2008
****************************************************************
Revisions and bug fixes:
*/
class LibFunction : public Function
{
public:
	
	// Library function pointer type definition
	typedef ArrayObj* (*FnPointer)(ArrayObj* pArguments);
	
	// Default constructor
	LibFunction(const std::string& name, FnPointer function, TypeMapFunc typeMapping = nullTypeMapping)
	: m_pHostFunc(function), 
	  m_pTypeMapFunc(typeMapping) 
	{ m_isProgFunction = false; m_funcName = name; }
	
	// Method to recursively copy this node
	LibFunction* copy() const { return new LibFunction(m_funcName, m_pHostFunc); }
	
	// Method to obtain a string representation of this node
	std::string toString() const { return "<LIBFUNCTION:" + m_funcName + ">"; }
	
	// Accessors to get a pointer to the host function
	FnPointer getHostFunc() const { return m_pHostFunc; }
	
	// Accessors to get a pointer to the type mapping function
	TypeMapFunc getTypeMapping() const { return m_pTypeMapFunc; }
	
private:

	// Pointer to host function
	FnPointer m_pHostFunc;
	
	// Pointer to type mapping function
	TypeMapFunc m_pTypeMapFunc;
};

/***************************************************************
* Class   : ProgFunction
* Purpose : Represent a program function
* Initial : Maxime Chevalier-Boisvert on October 29, 2008
****************************************************************
Revisions and bug fixes:
*/
class ProgFunction : public Function
{
public:
	
	// Parameter vector type definition
	typedef std::vector<SymbolExpr*, gc_allocator<SymbolExpr*> > ParamVector; 
	
	// Function vector type definition
	typedef std::vector<ProgFunction*, gc_allocator<ProgFunction*> > FuncVector;
	
	// Constructor
	ProgFunction(
		const std::string& name,
		const ParamVector& inParams,
		const ParamVector& outParams,
		const FuncVector& nestedFuncs,
		StmtSequence* stmts,
		bool isScript = false,
		bool isClosure = false
	);
	
	// Method to recursively copy this node
	ProgFunction* copy() const;
	
	// Method to obtain a string representation of this node
	std::string toString() const;
	
	// Method to create a temporary variable in this function
	SymbolExpr* createTemp();
	
	// Method to get all symbols used/read in this function
	Expression::SymbolSet getSymbolUses() const;
	
	// Method to get all symbols written/defined in this function
	Expression::SymbolSet getSymbolDefs() const;
	
	// Mutator to change the current function body
	void setCurrentBody(StmtSequence* pNewBody) { m_pCurrentBody = pNewBody; }

	// Mutator to set the script flag
	void setScript(bool script) { m_isScript = script; }
	
	// Mutator to set the closure flag
	void setClosure(bool closure) { m_isClosure = closure; }
	
	// Mutator to set the parent function pointer
	void setParent(ProgFunction* pParent) { m_pParent = pParent; }
	
	// Static method to set the local environment of a function
	static void setLocalEnv(ProgFunction* pFunc, Environment* pEnv) { pFunc->m_pLocalEnv = pEnv; }
	
	// Accessor to get the input parameters
	const ParamVector& getInParams() const { return m_inputParams; }
	
	// Accessor to get the output parameters
	const ParamVector& getOutParams() const { return m_outputParams; }
	
	// Accessor to get the nested functions
	const FuncVector& getNestedFuncs() const { return m_nestedFuncs; }
	
	// Accessor to get the original function body
	StmtSequence* getOrigBody() const { return m_pOrigBody; }
	
	// Accessor to get the current function body
	StmtSequence* getCurrentBody() const { return m_pCurrentBody; }
	
	// Static method to get the local environment
	static Environment* getLocalEnv(const ProgFunction* pFunc) { return pFunc->m_pLocalEnv; }
	
	// Accessor to get the script flag
	bool isScript() const { return m_isScript; }
	
	// Accessor to get the closure flag
	bool isClosure() const { return m_isClosure; }
	
	// Accessor to get the parent function pointer
	ProgFunction* getParent() const { return m_pParent; }
	
private:

	// Input parameters
	ParamVector m_inputParams;
	
	// Output parameters
	ParamVector m_outputParams;
	
	// Nested functions
	FuncVector m_nestedFuncs;
	
	// Original body of the function
	StmtSequence* m_pOrigBody;
	
	// Current version of the function body
	StmtSequence* m_pCurrentBody;
	
	// Local function environment
	Environment* m_pLocalEnv;
	
	// Indicates if the function is a script
	bool m_isScript;
	
	// Indicates if the function is a closure
	bool m_isClosure;
	
	// Pointer to the parent function (null if none)
	ProgFunction* m_pParent;
	
	// Next available temp variable id
	size_t m_nextTempId;
};

/***************************************************************
* Class   : FnHandleObj
* Purpose : Represent a function handle object
* Initial : Maxime Chevalier-Boisvert on February 25, 2009
****************************************************************
Revisions and bug fixes:
*/
class FnHandleObj : public DataObject
{
public:

	// Default constructor
	FnHandleObj(Function* pFunction) : m_pFunction(pFunction) { m_type = FN_HANDLE; }
	
	// Method to recursively copy this node
	FnHandleObj* copy() const { return new FnHandleObj(m_pFunction); }
	
	// Method to obtain a string representation of this node
	std::string toString() const { return "@" + ((m_pFunction->getFuncName() != "")? m_pFunction->getFuncName():"<anonymous function>"); }
	
	// Accessor to get the function pointer
	Function* getFunction() const { return m_pFunction; }
	
private:
	
	// Internal function pointer
	Function* m_pFunction;
};

#endif // #ifndef FUNCTIONS_H_
