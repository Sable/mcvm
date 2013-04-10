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
#include "exprstmt.h"

/***************************************************************
* Function: ExprStmt::copy()
* Purpose : Copy this IIR node recursively
* Initial : Maxime Chevalier-Boisvert on November 7, 2008
****************************************************************
Revisions and bug fixes:
*/
ExprStmt* ExprStmt::copy() const
{
	// Create and return a copy of this node
	return new ExprStmt((Expression*)m_pExpr->copy(), m_suppressOut);
}	

/***************************************************************
* Function: ExprStmt::toString()
* Purpose : Generate a text representation of this IIR node
* Initial : Maxime Chevalier-Boisvert on November 7, 2008
****************************************************************
Revisions and bug fixes:
*/
std::string ExprStmt::toString() const
{
	// Return the string representation of the expression
	return m_pExpr->toString();
}
