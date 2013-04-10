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
#ifndef CELLARRAYOBJ_H_
#define CELLARRAYOBJ_H_

// Header files
#include "matrixobjs.h"

// Cell array type definition
typedef MatrixObj<DataObject*> CellArrayObj;

// Template specialization of the matrix allocation method for cell arrays
template <> void MatrixObj<DataObject*>::allocMatrix();

// Template specialization of the string representation method for cell arrays
template <> std::string MatrixObj<DataObject*>::toString() const;

#endif // #ifndef CELLARRAYOBJ_H_ 
