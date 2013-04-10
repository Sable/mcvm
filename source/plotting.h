// =========================================================================== //
//                                                                             //
// Copyright 2009 Olivier Savary B.  and McGill University.                    //
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

#ifndef PLOTTING_H_
#define PLOTTING_H_

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <sys/time.h>
#include "stdlib.h"
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include "runtimebase.h"
#include "interpreter.h"
#include "matrixobjs.h"
#include "matrixops.h"
#include "chararrayobj.h"
#include "cellarrayobj.h"
#include "utility.h"
#include "process.h"
#include "filesystem.h"

class Plotting{
	
public:
	
	//constructor, deconstructor
	Plotting(ArrayObj* arguments);
	 
	virtual ~Plotting(){}
	
	//this method loop  the arguments to compose a mapping string, and call argIsValid to verify the validity of the arguments. 
	//return true if there is any option inputed
	void parsing();
	
	void callGnuplot();
	
	//this method parse the option and fill a string of option.
	void printOpt();
	
	//this method save the matrices to the file "gnuplotdata.dat", in the tmp folder.  it  also save the options to "gnuplotrun.p".
	void printData();
	
	/* This method parse the string of arguments to determine the amount of line to be plotted 
	 * (in trio, x\y\o or x\y or y\o or y)     */
	void trioNparsing();
		

private:

	ArrayObj* argumentList;
	
	bool optFlag;

	CharArrayObj* pOptString;
	
	DimVector inSizeX;
	DimVector inSizeY;

	int X_lenght;//temp number
	int Y_lenght;//temp number
	int LS_number;//temp number
	
	int X_number;//number of matrix that are used as X
	int Y_number;//number of matrix that are used as Y
	int opt_number;//number of linestyle
	unsigned int trioID_number;//number of TrioID

	unsigned int numberofArgs;
	struct trioID { int X, Y,  opt; }; 
	std::vector<trioID> trioRef;

	
	MatrixF64Obj* MatrixX;
	MatrixF64Obj* MatrixY;

	std::string argMappingstr;
	std::string trioMappingstr;
	
	int LineWidth;
	int MarkerEdgeColor;
	int MarkerFaceColor;
	int MarkerSize;
	
	char tempOption;
	std::string datafile ;
	std::string name;
	std::string optionStrings;
	std::string optfile  ;
	std::string dataoptBuffer;
	std::string graphStyle;

	//this method check the validity of the inputed matrices.
	void checkMatrix(int arg1, int arg2);

	//this method check if the argument of rank [argRank] is a matrix or a string, and return a character accordingly('m' for matrix, 'o' for string, 'e' for error/other)
	char argIsValid(int argRank);
};

#endif /*PLOTTING_H_*/
