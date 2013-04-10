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

// Header files
#include <cstdio>
#include "spreadsheet.h"
#include "utility.h"

/***************************************************************
* Function: SpreadSheet::SpreadSheet()
* Purpose : Constructor for spreadsheet class
* Initial : Maxime Chevalier-Boisvert on April 10, 2007
****************************************************************
Revisions and bug fixes:
*/
SpreadSheet::SpreadSheet()
{
}

/***************************************************************
* Function: SpreadSheet::~SpreadSheet()
* Purpose : Destructor for spreadsheet class
* Initial : Maxime Chevalier-Boisvert on April 10, 2007
****************************************************************
Revisions and bug fixes:
*/
SpreadSheet::~SpreadSheet()
{
}

/***************************************************************
* Function: SpreadSheet::loadCSV()
* Purpose : Load a CSV spreadsheet file
* Initial : Maxime Chevalier-Boisvert on April 10, 2007
****************************************************************
Revisions and bug fixes:
*/
bool SpreadSheet::loadCSV(const std::string& FileName)
{
    // Attempt to open the file in read-only text mode
    FILE* pFile = fopen(FileName.c_str(), "r");
    
    // Ensure that the file was opened
    if (!pFile)
        return false;
    
    // Declare a character for reading
    int Char = '\0';
    
    // Variable to tell if we are inside a string
    bool InString = false;
    
    // Variable to tell that we just finished a string
    bool EndString = false;
        
    // Variable to tell that we just ended a cell
    bool EndCell = true;
        
    // Declare a variable for the current row
    Row Row;
    
    // Declare a string for the current cell contents
    std::string Cell;
    
    // Until we are done parsing the file
    for (;;)
    {
        // Read a character from the file
        Char = fgetc(pFile);
        
        // If this is the end of the file
        if (Char == EOF)
        {
            // If the last cell is not empty, add it
            if (Cell != "")
                Row.push_back(Cell);            
            
            // If the last row is not empty, add it
            if (Row.size() != 0)
                m_grid.push_back(Row);
                    
            // Stop parsing
            break;
        }
        
        // If this is the end of a row
        if (Char == '\n')
        { 
            // No longer at the end of a string
            EndString = false;

            // Now at the end of a cell
            EndCell = true;
                
            // If the last cell is not empty, add it
            if (Cell != "")
                Row.push_back(Cell);
                
            // Clear the cell contents
            Cell.clear();
                
            // Add this row to the table
            m_grid.push_back(Row);
            
            // Clear the row contents
            Row.clear();
            
            // Move on to the next character
            continue;
        }
        
        // If this is the beginning or the end of a string    
        if (Char == '\"')
        {
            // If we are not inside a string
            if (!InString)
            {
                // No longer at the end of a cell
                EndCell = false;
                
                // We now are inside a string
                InString = true;
            }
            else
            {
                // We are no longer inside a string
                InString = false;
            
                // Note that we just finished a string
                EndString = true;
            }
            
            // Move on to the next character
            continue;
        }
        
        // If this is a cell separator
        if (Char == ',' && !InString)
        {
            // No longer after end of string
            EndString = false;
            
            // Now at the end of a cell
            EndCell = true;
            
            // Add this cell to the row
            Row.push_back(Cell);
            
            // Clear the cell contents
            Cell.clear();
            
            // Move on to the next character
            continue;
        }
        
        // If this is whitespace after a string or a comma
        if (isWhiteSpace(Char) && (EndCell || EndString))
        {
            // Skip the whitespace
            continue;
        }
        else
        {
            // No longer at the end of a cell
            EndCell = false;
        }
        
        // If this is text just after a string
        if (EndString)
        {
            // Skip this text
            continue;
        }
        
        // Add the text to this cell
        Cell += Char;        
    }    
    
    // Close the file
    fclose(pFile);
    
    // Nothing went wrong
    return true;
}

/***************************************************************
* Function: SpreadSheet::saveCSV()
* Purpose : Produce a CSV spreadsheet file
* Initial : Maxime Chevalier-Boisvert on April 10, 2007
****************************************************************
Revisions and bug fixes:
*/
bool SpreadSheet::saveCSV(const std::string& FileName) const
{
    // Attempt to open the file in write-only text mode
    FILE* pFile = fopen(FileName.c_str(), "w");
    
    // Ensure that the file was opened
    if (!pFile)
        return false;
    
    // For each row of the grid
    for (size_t i = 0; i < m_grid.size(); ++i)
    {
        // Get a reference to this row
        const Row& Row = m_grid[i];
        
        // For each cell of this row
        for (size_t j = 0; j < Row.size(); ++j)
        {
            // Add an opening quote
            fputs("\"", pFile);
            
            // Output the cell content
            fputs(Row[j].c_str(), pFile);
            
            // Add a closing quote
            fputs("\"", pFile);
            
            // If this is not the last cell, add a comma separator
            if (j != Row.size() - 1)
                fputs(",", pFile);
        }
        
        // Add an end of line character
        fputs("\n", pFile);
    }
    
    // Close the file
    fclose(pFile);
    
    // Nothing went wrong
    return true;
}

/***************************************************************
* Function: SpreadSheet::readCell()
* Purpose : Get a value from a cell
* Initial : Maxime Chevalier-Boisvert on April 10, 2007
****************************************************************
Revisions and bug fixes:
*/
std::string SpreadSheet::readCell(uint32 X, uint32 Y) const
{
    // If this cell has a value set
    if (Y < m_grid.size() && X < m_grid[Y].size())
    {
        // Return the value of the cell
        return m_grid[Y][X];
    }
    else
    {
        // Otherwise, return the empty string
        return "";
    }
}

/***************************************************************
* Function: SpreadSheet::writeCell()
* Purpose : set a value in a cell
* Initial : Maxime Chevalier-Boisvert on April 10, 2007
****************************************************************
Revisions and bug fixes:
*/
void SpreadSheet::writeCell(uint32 X, uint32 Y, const std::string& Value)
{
    // If the row does not exist
    if (Y >= m_grid.size())
    {
        // create new rows accordingly
        m_grid.resize(Y + 1);
        
        // create new cells in the row
        m_grid[Y].resize(X + 1);
    }
    
    // Otherwise, if the column does not exists
    else if (X >= m_grid[Y].size())
    {
        // create new cells in the row
        m_grid[Y].resize(X + 1);
    }
    
    // set the value
    m_grid[Y][X] = Value;
}

/***************************************************************
* Function: SpreadSheet::findColumn()
* Purpose : Find a column by name
* Initial : Maxime Chevalier-Boisvert on April 11, 2007
****************************************************************
Revisions and bug fixes:
*/
int SpreadSheet::findColumn(const std::string& Name) const
{
    // Ensure the grid has at least one row
    if (m_grid.size() == 0)
        return -1;
    
    // For each cell of the first row
    for (int i = 0; i < (int)m_grid[0].size(); ++i)
    {
        // If this is the cell we are looking for, return its index
        if (compareNoCase(m_grid[0][i], Name))
            return i;
    }

    // Column not found
    return -1;
}
