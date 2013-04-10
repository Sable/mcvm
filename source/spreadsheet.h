// =========================================================================== //
//                                                                             //
// Copyright 2007 Maxime Chevalier-Boisvert and McGill University.             //
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
/***************************************************************
*
* The programming code contained in this file was written
* by Maxime Chevalier-Boisvert. For information, please
* contact the author.
*
* Contact information:
* e-mail: mcheva@cs.mcgill.ca
* phone : 1-514-935-2809
*
***************************************************************/
// Definitions
#ifndef __SPREADSHEETS_H__
#define __SPREADSHEETS_H__

// Header files
#include <string>
#include <vector>
#include "platform.h"

/***************************************************************
* Class   : SpreadSheet
* Purpose : Manipulate spreadsheet information
* Initial : Maxime Chevalier-Boisvert on April 10, 2007
****************************************************************
Revisions and bug fixes:
*/
class SpreadSheet
{
public:

    // Constructor and destructor
    SpreadSheet();
    ~SpreadSheet();

    // Method to load a CSV file
    bool loadCSV(const std::string& FileName);

    // Method to save a CSV file
    bool saveCSV(const std::string& FileName) const;

    // Method to get the value of a specific cell
    std::string readCell(uint32 X, uint32 Y) const;

    // Method to set the value of specific cell
    void writeCell(uint32 X, uint32 Y, const std::string& Value);

    // Method to find a column by name
    int findColumn(const std::string& Name) const;

    // Accessor to get the number of rows
    uint32 getNumRows() { return (uint32)m_grid.size(); }
    
    // Accessor to get the number of cells in a row
    uint32 getRowLength(uint16 j) { return (j >= m_grid.size())? 0:(uint32)m_grid[j].size(); }

private:

    // Row type definition
    typedef std::vector<std::string> Row;

    // Grid type definition
    typedef std::vector<Row> Grid;
    
    // Internal storage grid
    Grid m_grid;
};

#endif // #ifndef __SPREADSHEETS_H__
