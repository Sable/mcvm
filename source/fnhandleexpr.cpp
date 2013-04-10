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

// Header files
#include "fnhandleexpr.h"

/***************************************************************
* Function: FnHandleExpr::replaceSubExpr()
* Purpose : Replace a sub-expression
* Initial : Maxime Chevalier-Boisvert on March 17, 2008
****************************************************************
Revisions and bug fixes:
*/
void FnHandleExpr::replaceSubExpr(size_t index, Expression* pNewExpr)
{
	// Ensure the index is 0
	assert (index == 0);
	
	// Ensure the new expression is a symbol
	assert (pNewExpr->getExprType() == Expression::SYMBOL);
		
	// Replace the symbol expression
	m_pSymExpr = (SymbolExpr*)pNewExpr;
}
