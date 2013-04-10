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

#include "plotting.h"

//constructor, deconstructor
Plotting::Plotting(ArrayObj* arguments):
  LS_number(0),LineWidth(0), MarkerEdgeColor(0),MarkerFaceColor(0), MarkerSize(0){
  char* autogen;
  tmpnam(autogen);
  name = (std::string)autogen;
  argumentList = arguments ;
};


//this method loop  the arguments to compose a mapping string, and call argIsValid to verify the validity of the arguments. 
//return true if there is any option inputed
void Plotting::parsing(){
  
  unsigned int i=0;
  char argType;
  std::ofstream gnuplotdatafile;
  
  numberofArgs = argumentList ->getSize();
  
  
  //function that look if the argument is a matrix or a string, and return of character according to the result ('m' for matrix, 'o' for option string, 'e' for error)
  argMappingstr="";
  do{
    argType = argIsValid(i);
    if(argType == 's'){
      argMappingstr += "ss";
      i++;
    }else{
      argMappingstr+= argType;
    }
  }while(++i<numberofArgs);
  
  trioNparsing();
  
  
  
  
  
  
}

void Plotting::callGnuplot(){
  
  std::string optfile = "gnuplotrun.p";
  //std::string optfile =  "~/tmp/mcvmtemp/gnuplotrun"+name+".p";
  //popen is a nonstandart function used to open pipe.
  std::string gnuplotcall = "gnuplot "+optfile+" -persist";
  const char* gnuplotchar = gnuplotcall.c_str();
  FILE *gnuplot = popen(gnuplotchar,"w");
  pclose(gnuplot);
}



//this method parse the option and fill a string of option.
void Plotting::printOpt(){
  
  CharArrayObj* pOptString;
  unsigned int i,j,k;
  std::string optStr;//unparsed string of matlab option
  std::string bufferOptStr="";//Buffer of gnuplot plot-option string
  std::string color="";
  std::string lstyle="";
  std::string mstyle="";
  LS_number=1;
  bool optParsed;
  //for each arguments
  for(k=0;k< argMappingstr.size();k++){
    
    //if the arg is an option	
    if(argMappingstr[k]=='o'){
      bufferOptStr += "\n";
      // Get a pointer to the option string
      pOptString = (CharArrayObj*)argumentList->getObject(k);
      optStr = pOptString->getString();
      bufferOptStr +="set style line ";
      bufferOptStr += ::toString(LS_number);
      LS_number++;
      //2 pass -> first to parse, second to print
      optParsed=false;
      for(j=0; j<2; j++){
	for( i=0; i< optStr.size() ;i++){
	  
	  if(optStr[i]=='-' && optStr[i+1] == '-'){
	    //Dashed line
	    optParsed?bufferOptStr+=" lt 2":lstyle= "--";
	    i++;
	  }else if(optStr[i]== '-' && optStr[i+1] == '.'){
	    //Dash-dot line
	    optParsed?bufferOptStr+=" lt 5":lstyle="-.";
	    i++;
	  }else if(optStr[i]=='-'){
	    //Solid line
	    optParsed? bufferOptStr+="": lstyle="-";
	  }else if(optStr[i]== ':'){
	    //dotted line
	    optParsed?bufferOptStr+=" lt 4":lstyle=":";
	    
	  }else if(optStr[i]=='+'){
	    //marker +
	    optParsed?bufferOptStr+=" pt 1":mstyle="+";
	    
	  }else if(optStr[i]=='o'){
	    //marker o
	    optParsed?bufferOptStr+=" pt 6":mstyle="o";
	    
	  }else if(optStr[i]=='.'){
	    //marker point
	    optParsed?bufferOptStr+=" pt 0":mstyle=".";
	    
	  }else if(optStr[i]=='x'){
	    //marker cross
	    optParsed?bufferOptStr+=" pt 2":mstyle="x";
	    
	  }else if(optStr[i]=='s'){
	    //marker square
	    optParsed?bufferOptStr+=" pt 4":mstyle="s";
	    
	  }else if(optStr[i]=='d'){
	    //marker diamond
	    optParsed?bufferOptStr+=" pt 12":mstyle="d";
	    
	  }else if(optStr[i]=='^'){
	    //marker upward triangle
	    optParsed?bufferOptStr+=" pt 8":mstyle="^";
	    
	  }else if(optStr[i]=='v'){
	    //marker downward triangle
	    optParsed?bufferOptStr+=" pt 9":mstyle="v";
	    
	  }else if(optStr[i]=='>'){
	    //marker right triangle
	    optParsed?bufferOptStr+=" pt 10":mstyle=">";
	    
	  }else if(optStr[i]=='<'){
	    //marker left triangle
	    optParsed?bufferOptStr+=" pt 11":mstyle="<";
	    
	  }else if(optStr[i]=='p'){
	    //marker pentagram					
	  }else if(optStr[i]=='h'){
	    //marker hexagram
	  }else if(optStr[i]=='r'){
	    //color red
	    optParsed?bufferOptStr+=" lc 1":color="r";
	    
	  }else if(optStr[i]=='g'){
	    //color green
	    optParsed?bufferOptStr+=" lc 2":color="g";
	    
	  }else if(optStr[i]=='b'){
	    //color blue
	    optParsed?bufferOptStr+=" lc 3":color="b";
	    
	  }else if(optStr[i]=='c'){
	    //color Cyan
	    optParsed?bufferOptStr+=" lc 5":color="c";
	    
	  }else if(optStr[i]=='m'){
	    //color magenta
	    optParsed?bufferOptStr+=" lc 4":color="m";
	  }else if(optStr[i]=='y'){
	    //color yellow
	    optParsed?bufferOptStr+=" lc 6":color="y";
	    
	  }else if(optStr[i]=='k'){
	    //color black
	    optParsed?bufferOptStr+=" lc 7":color="k";
	    
	  }else if(optStr[i]=='w'){
	    //color white->grey
	    optParsed?bufferOptStr+=" lc 9":color="w";
	  }
	}
	//Determine if this line style uses point, line or a combinaison of the two.
	if(j==0){
	  if(lstyle==""){
	    graphStyle= "points";
	  }else if(mstyle==""){
	    graphStyle = "lines";
	  }else{
	    graphStyle = "linespoints";
	  }
	}
	optStr=lstyle+mstyle+color;
	
	optParsed=true;
      }
      
      
      if(LineWidth != 0)bufferOptStr+="linewidth"+::toString(LineWidth);
      if(MarkerSize != 0)bufferOptStr+="pointsize"+::toString(MarkerSize);
      bufferOptStr += "\n";
    }
  }
  
  dataoptBuffer = bufferOptStr;
  
  
}


//this method save the matrices to the file "gnuplotdata.dat", in the tmp folder.  it  also save the options to "gnuplotrun.p".
void Plotting::printData(){
  std::string dataBuffer="";
  std::stringstream ss;
  //std::string datafile = "~/tmp/mcvmtemp/gnuplotdata"+name+".dat";
  //std::string optfile =  "~/tmp/mcvmtemp/gnuplotrun"+name+".p";
  std::string datafile= "gnuplotdata.dat";
  std::string optfile = "gnuplotrun.p";
  trioID currentTrio;
  const char* datachar = datafile.c_str();
  const char* optchar = optfile.c_str();
  MatrixF64Obj* currentMatrix;
  //store matrix in gnuplotdata.dat
  
  
  dataBuffer += "#Data for gnuplot, as received and saved by mclab  \n";
  currentMatrix = (MatrixF64Obj*)(BaseMatrixObj*)argumentList->getObject(0);
  inSizeX = currentMatrix->getSize();	
  
  // for each row(point)
  for(unsigned int k=1;k<inSizeX[1];k++){
    dataBuffer += "\n";
    for(unsigned int i=0;i<trioMappingstr.size();i++){
      
      
      if(trioMappingstr[i] == 'x'){
	currentMatrix = (MatrixF64Obj*)(BaseMatrixObj*)argumentList->getObject(i);
	inSizeX = currentMatrix->getSize();		
	
	//for each dimension of the current matrix
	
	for(unsigned int j = 1;j<inSizeX.size();j++){
	  // for each column
	  
	  ss << std::setprecision(15) << currentMatrix->getElem2D(j,k);
	  dataBuffer += ss.str() + "\t";
	  ss.str("");
	  
	}
      }else if(trioMappingstr[i] == 'y'){
	currentMatrix = (MatrixF64Obj*)(BaseMatrixObj*)argumentList->getObject(i);
	inSizeY = currentMatrix->getSize();
	
	//for each dimension of the current matrix
	for(unsigned int j = 1;j<inSizeY.size();j++){
	  // for each column
	  ss << std::setprecision(15) << currentMatrix->getElem2D(j,k);
	  dataBuffer += ss.str() + "\t";
	  ss.str("");
	  
	}
      }else{
	continue;
      }
      
    }
  }
  //print the buffer of data in a file
  std::ofstream gnuplotdatafile(datachar);
  gnuplotdatafile << dataBuffer;
  gnuplotdatafile.close();
  
  
  for(unsigned int i= 0;i < trioID_number;i++){
    
    //get a Trio
    currentTrio = trioRef[i];
    //construct the call to the plot func of gnuplot
    if(i==0){ 
      dataoptBuffer += "plot \""+datafile+ "\" " ;
    }else{
      dataoptBuffer+= ", \""+datafile+ "\"" ;
    }
    dataoptBuffer +=  " using "+::toString(currentTrio.X);
    if(currentTrio.Y != 0){
      dataoptBuffer += ":";
      dataoptBuffer +=  ::toString(X_number +currentTrio.Y);
      dataoptBuffer += " ";
    }
    if(currentTrio.opt !=0)dataoptBuffer += "  with "+graphStyle+" ls " + ::toString(currentTrio.opt);
    
  }
  
  //print the plot call in a file
  gnuplotdatafile.open(optchar);
  gnuplotdatafile << dataoptBuffer;
  gnuplotdatafile.close();
  
  return;
}


/* This method parse the string of arguments to determine the amount of line to be plotted 
 * (in trio, x\y\o or x\y or y\o or y)     */
void Plotting::trioNparsing(){
  MatrixF64Obj* pMatrixArg;
  int passNumber= 0;
  X_number=0;
  Y_number=0;
  opt_number=0;
  Y_lenght=0;
  trioID_number=0;
  //int[3] ref_array;
  unsigned int i = 0;
  trioMappingstr = "";
  trioRef.resize(numberofArgs);
  
  do{
    
    //1st: is [i] a matrix?-> X : quit
    if(argMappingstr[i]=='m'){
      //[i+1] exist?
      if((i+1)<numberofArgs){
	//2nd: is [i+1] a matrix? -> Y : option
	if(argMappingstr[i+1]=='m'){
	  //[i+2] exist?
	  if((i+2)<numberofArgs){
	    
	    //3rd: is [i+2] a matrix? -> Quit : Option
	    if(argMappingstr[i+2]=='m'){
	      
	      checkMatrix(i,i+1);
	      trioID ref_array = {++X_number,++Y_number,0};
	      trioRef[trioID_number]=ref_array;
	      
	      trioID_number++;
	      trioMappingstr += "xy";
	      i++;
	      
	    }else if(argMappingstr[i+2]=='o'){
	      
	      
	      checkMatrix(i,i+1);
	      
	      trioID ref_array = {++X_number,++Y_number,++opt_number};
	      
	      trioRef[trioID_number]=ref_array;
	      
	      trioID_number++;
	      trioMappingstr += "xyo";
	      i++;i++;
	      
	    }
	  }else{
	    checkMatrix(i,i+1);
	    
	    trioID ref_array = {++X_number,++Y_number,0};
	    trioRef[trioID_number]=ref_array;
	    
	    trioID_number++;
	    trioMappingstr += "xy";
	    i++;
	    //quit
	  }
	  
	  pMatrixArg = (MatrixF64Obj*)((BaseMatrixObj*)argumentList->getObject(i));
	  Y_lenght = pMatrixArg->getSize().size();
	  if(Y_lenght>1 && Y_lenght<passNumber){
	    passNumber++;
	    X_number--;
	    i--;
	  }
	  
	}else if(argMappingstr[i+1]=='o'){
	  
	  trioID ref_array  ={++X_number,0,++opt_number};
	  trioRef[trioID_number]=ref_array;
	  trioID_number++;
	  trioMappingstr += "xo";
	  i++;
	  
	}
      }else{
	
	trioID ref_array ={++X_number,0,0};
	trioRef[trioID_number]=ref_array;
	trioID_number++;
	trioMappingstr += "x";
      }
    }else if(argMappingstr[i]=='o'){
      trioMappingstr += "o";
    }else if(argMappingstr[i]=='s'){
      trioMappingstr += "s";
    }
    
    
    
    
  }while(++i<numberofArgs);
  
}

//this method check the validity of the inputed matrices.
void Plotting::checkMatrix(int arg1, int arg2){
  unsigned int i;
  MatrixF64Obj* pMatrixArg1;
  MatrixF64Obj* pMatrixArg2;
  
  pMatrixArg1 = (MatrixF64Obj*)((BaseMatrixObj*)argumentList->getObject(arg1));
  inSizeX = pMatrixArg1->getSize();
  
  pMatrixArg2 = (MatrixF64Obj*)((BaseMatrixObj*)argumentList->getObject(arg2));
  inSizeY = pMatrixArg2->getSize(); 
  //TODO -> this run error doesnt seems to work, get thrown even when the vectors are fine 
  for(i=1;i<inSizeX.size();i++){
    //if(inSizeX[i]!= inSizeX[0])throw RunError("Vectors must be of the same length");
  }
  for(i=0;i<inSizeY.size();i++){
    //if(inSizeY[i] != inSizeX[0]) throw RunError("Vectors must be of the same length");
  }
  
}

//this method check if the argument of rank [argRank] is a matrix or a string, and return a character accordingly('m' for matrix, 'o' for string, 'e' for error/other)
char Plotting::argIsValid(int argRank){
  
  int argRankNext = argRank+1;
  
  if(argumentList->getObject(argRank)->getType()== DataObject::MATRIX_F64 ){
    
    
    return 'm';
    
    
  }else if( argumentList->getObject(argRank)->getType()== DataObject::CHARARRAY){
    
    // Get a pointer to the arrayopyion
    optionStrings = "";
    pOptString = (CharArrayObj*)argumentList->getObject(argRank);
    optionStrings += pOptString->getString();
    
    if(optionStrings[0]=='M'||optionStrings[0]=='L'){
      
      if(optionStrings=="LineWidth"){
	tempOption = ((CharArrayObj*)argumentList ->getObject(argRankNext))->getScalar();
	// LineWidth = atoi(tempOption.c_str());
	LineWidth = (int) tempOption;
	return 's';
	
      }else if(optionStrings=="MarkerEdgeColor"){
	tempOption = ((CharArrayObj*)argumentList ->getObject(argRankNext))->getScalar();
	//MarkerEdgeColor = atoi(tempOption.c_str());
	MarkerEdgeColor = (int) tempOption;
	return 's';
	
      }else if(optionStrings=="MarkerFaceColor"){
	tempOption = ((CharArrayObj*)argumentList ->getObject(argRankNext))->getScalar();
	//MarkerFaceColor= atoi(tempOption.c_str());
	MarkerFaceColor = (int) tempOption;
	return 's';
	
      }else if(optionStrings=="MarkerSize"){
	tempOption = ((CharArrayObj*)argumentList ->getObject(argRankNext))->getScalar();
	//MarkerSize = atoi(tempOption.c_str());
	MarkerSize = (int) tempOption;
	return 's';				
      }
    }
    return 'o';
    
  }else{
    throw RunError("Expected matrix or string as argument "+argRank);
    return 'e';
  }
  
}
