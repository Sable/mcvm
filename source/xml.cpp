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

// Header files
#include <cassert>
#include <cstdio>
#include <ctype.h>
#include <sstream>
#include <iostream>
#include "xml.h"
#include "utility.h"
#include "configmanager.h"

// Include this in the XML namespace
namespace XML
{
	/***************************************************************
	* Function: TextPos::toString()
	* Purpose : Produce a string representation of the position
	* Initial : Maxime Chevalier-Boisvert on October 19, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	std::string TextPos::toString() const
	{
		// Return a string with the line and column numbers
		return "(" + ::toString(m_line) + "," + ::toString(m_column) + ")";
	}
	
	/***************************************************************
	* Function: ParseError::toString()
	* Purpose : Produce a string representation of the error
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/	
	std::string ParseError::toString() const
	{
		// Create a string to store the output
		std::string output;
		
		// If a text position was specified
		if (m_textPos.getLine() != 0)
		{
			// Add the text position to the output
			output += m_textPos.toString() + " ";
		}
		
		// Add the error text to the output
		output += m_errorText;
		
		// Return the string representation
		return output;
	}

	/***************************************************************
	* Function: Text::toString()
	* Purpose : Produce a string representation of this node
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	std::string Text::toString(bool indent, size_t level) const
	{
		// Create a string to store the string representation
		std::string output;

		// If the output should be indented
		if (indent)
		{
			// If this is not the first indentation level
			if (level > 0)
			{
				// Begin a new line
				output += "\n";
			}

			// For each indentation level
			for (size_t i = 0; i < level; ++i)
			{
				// Add two spaces
				output += "  ";
			}
		}

		// Escape the text and add it to the output
		output += escapeString(m_text);

		// Return the string representation
		return output;
	}
	
	/***************************************************************
	* Function: RawData::toString()
	* Purpose : Produce a string representation of this node
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	std::string RawData::toString(bool indent, size_t level) const
	{
		// Create a string to store the string representation
		std::string output;

		// If the output should be indented
		if (indent)
		{
			// If this is not the first indentation level
			if (level > 0)
			{
				// Begin a new line
				output += "\n";
			}

			// For each indentation level
			for (size_t i = 0; i < level; ++i)
			{
				// Add two spaces
				output += "  ";
			}
		}

		// Add the text to the output directly
		output += "<![CDATA[" + m_contents + "]]>";

		// Return the string representation
		return output;
	}
	
	/***************************************************************
	* Function: Element::~Element()
	* Purpose : Destructor for the XML element class
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	Element::~Element()
	{
		// Delete each child node
		for (std::vector<Node*>::const_iterator itr = m_children.begin(); itr != m_children.end(); ++itr)
			delete (*itr);
	}
	
	/***************************************************************
	* Function: Element::setStringAttrib()
	* Purpose : Set the value of an attribute
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	void Element::setStringAttrib(const std::string& name, const std::string& value)
	{
		// Ensure that the name is not empty
		assert (name.length() > 0);

		// Set the attribute value
		m_attributes[name] = value;
	}

	/***************************************************************
	* Function: Element::getStringAttrib()
	* Purpose : Get the string value of an attribute
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	const std::string& Element::getStringAttrib(const std::string& name) const
	{
		// Attempt to find the attribute
		std::map<std::string, std::string>::const_iterator attribItr = m_attributes.find(name);

		// If the attribute was not found, throw an exception
		if (attribItr == m_attributes.end())
			throw ParseError("Attribute \"" + name + "\" not found in \"" + m_name + "\" tag", m_textPos);

		// Return the attribute's value
		return attribItr->second;	
	}

	/***************************************************************
	* Function: Element::getStringAttrib()
	* Purpose : Get the floating-point value of an attribute
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	float Element::getFloatAttrib(const std::string& name) const
	{
		// Declare a stringstream object for parsing
		std::stringstream stringStream;
		
		// Attempt to find the string value of the attribute
		std::string stringAttrib = getStringAttrib(name);

		// Set the string value of the attribute
		stringStream.str(stringAttrib);

		// Declare an float to extract the integer value
		float floatValue;

		// Extract the integer value
		stringStream >> floatValue;

		// Return the float value
		return floatValue;
	}

	/***************************************************************
	* Function: Element::getStringAttrib()
	* Purpose : Get the integer value of an attribute
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	int Element::getIntAttrib(const std::string& name) const
	{
		// Declare a stringstream object for parsing
		std::stringstream stringStream;
		
		// Attempt to find the string value of the attribute
		std::string stringAttrib = getStringAttrib(name);

		// Set the string value of the attribute
		stringStream.str(stringAttrib);

		// Declare an integer to extract the integer value
		int intValue;

		// Extract the integer value
		stringStream >> intValue;

		// Return the integer value
		return intValue;		
	}

	/***************************************************************
	* Function: Element::getBoolAttrib()
	* Purpose : Get the boolean value of an attribute
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	bool Element::getBoolAttrib(const std::string& name) const
	{
		// Attempt to find the string value of the attribute
		std::string stringAttrib = getStringAttrib(name);

		// Compute the boolean value
		bool boolValue = (
			stringAttrib == "true" ||
			stringAttrib == "True" ||
			stringAttrib == "TRUE" ||
			stringAttrib == "1"
		);

		// Return the boolean value
		return boolValue;
	}
	
	/***************************************************************
	* Function: Element::getChildNode()
	* Purpose : Get a specific child node
	* Initial : Maxime Chevalier-Boisvert on November 18, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	Node* Element::getChildNode(size_t index) const
	{
		// If the index is not valid, throw an exception
		if (index >= m_children.size())
			throw ParseError("Missing child element", m_textPos);
		
		// Return a pointer to the child element
		return m_children[index];
	}
	
	/***************************************************************
	* Function: Element::getChildElement()
	* Purpose : Get a child node known to be an element
	* Initial : Maxime Chevalier-Boisvert on November 18, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	Element* Element::getChildElement(size_t index) const
	{
		// Get a pointer to the child element
		Node* pChild = getChildNode(index);
		
		// If this is not an element, throw an exception
		if (pChild->getType() != ELEMENT)
			throw ParseError(
				"Invalid node type for child #" + ::toString(index+1) + ", expected child element",
				m_textPos
			);
		
		// Return a pointer to an XML element
		return (Element*)pChild;	
	}
	
	/***************************************************************
	* Function: Element::copy()
	* Purpose : Recursively copy this node or subtree
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	Node* Element::copy() const
	{
		// Create a vector to store copies of child nodes
		std::vector<Node*> childCopies;
		
		// Copy all the child nodes
		for (std::vector<Node*>::const_iterator itr = m_children.begin(); itr != m_children.end(); ++itr)
			childCopies.push_back((*itr)->copy());
		
		// Create a copy of this element
		return new Element(
			m_name,
			m_attributes,
			childCopies,
			m_textPos,
			m_leaf
		);
	}
	
	/***************************************************************
	* Function: Element::toString()
	* Purpose : Produce a string representation of an XML element
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	std::string Element::toString(bool indent, size_t level) const
	{
		// Create a string to store the string representation
		std::string output;

		// If the output should be indented
		if (indent)
		{
			// If this is not the first indentation level
			if (level > 0)
			{
				// Begin a new line
				output += "\n";
			}

			// For each indentation level
			for (size_t i = 0; i < level; ++i)
			{
				// Add two spaces
				output += "  ";
			}
		}

		// Begin the opening tag
		output += "<" + m_name;

		// For each attribute of this tag
		for (std::map<std::string, std::string>::const_iterator itr = m_attributes.begin(); itr != m_attributes.end(); ++itr)
		{
			// Add this attribute to the string
			output += " " + itr->first + "=\"" + escapeString(itr->second) + "\"";
		}

		// If this is a leaf tag
		if (m_leaf)
		{
			// End the leaf tag
			output += " />";
		}

		// Otherwise, this is not a leaf tag
		else
		{
			// End the opening tag
			output += ">";

			// For each child node
			for (std::vector<Node*>::const_iterator itr = m_children.begin(); itr != m_children.end(); ++itr)
			{
				// Add this child tag to the string
				output += (*itr)->toString(indent, level + 1);
			}

			// If the output should be indented
			if (indent)
			{
				// Begin a new line
				output += "\n";

				// For each indentation level
				for (size_t i = 0; i < level; ++i)
				{
					// Add two spaces
					output += "  ";
				}
			}

			// Add the closing tag
			output += "</" + m_name + ">";
		}

		// Return the string representation
		return output;
	}
	
	/***************************************************************
	* Function: Declaration::toString()
	* Purpose : Produce a string representation of this node
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	std::string Declaration::toString(bool indent, size_t level) const
	{
		// Create a string to store the string representation
		std::string output;

		// If the output should be indented
		if (indent)
		{
			// If this is not the first indentation level
			if (level > 0)
			{
				// Begin a new line
				output += "\n";
			}

			// For each indentation level
			for (size_t i = 0; i < level; ++i)
			{
				// Add two spaces
				output += "  ";
			}
		}

		// Begin the tag opening
		output += "<?xml";

		// For each attribute of this tag
		for (std::map<std::string, std::string>::const_iterator itr = m_attributes.begin(); itr != m_attributes.end(); ++itr)
		{
			// Add this attribute to the string
			output += " " + itr->first + "=\"" + escapeString(itr->second) + "\"";
		}

		// Close the tag
		output += "?>";

		// Return the string representation
		return output;
	}
	
	/***************************************************************
	* Function: Document::Document()
	* Purpose : Copy constructor for document class
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	Document::Document(const Document& document)
	{
		// Use the assignment operator implementation
		*this = document;
	}
	
	/***************************************************************
	* Function: Document::toString()
	* Purpose : Produce a string representation of the document
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	std::string Document::toString(bool indent)
	{
		// Declare a string to store the output
		std::string output;
		
		// If the declaration is present
		if (m_pDecl)
		{
			// Add the declaration to the output
			output += m_pDecl->toString(true);
			
			// If indentation is enabled, add a newline
			if (indent)
				output += "\n";
		}
		
		// If the XML tree is present
		if (m_pTree)
		{
			// Add the tree to the output
			output += m_pTree->toString(true);
			
			// If indentation is enabled, add a newline
			if (indent)
				output += "\n";
		}
		
		// Return the output
		return output;
	}
	
	/***************************************************************
	* Function: Document::operator = ()
	* Purpose : Assignment operator
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	Document& Document::operator = (const Document& document)
	{
		// Copy the document declaration, if present
		m_pDecl = document.m_pDecl? (Declaration*)document.m_pDecl->copy():NULL;
		
		// Copy the XML tree, if present
		m_pTree = document.m_pTree? (Element*)document.m_pTree->copy():NULL;
		
		// Return a reference to this object
		return *this;
	}
	
	/***************************************************************
	* Function: Parser::parseString()
	* Purpose : Parse an XML input string
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	Document Parser::parseString(const std::string& xmlString)
	{
		// Declare a variable for the current character index
		size_t charIndex = 0;

		// Declare a vector for the text positions in the streamlined input
		PosVector positions;

		// streamline the input raw xml string
		std::string input = streamline(xmlString, positions);

		// If the verbose output flag is set
		if (ConfigManager::s_verboseVar.getBoolValue() == true)
		{
			// Output the raw and streamlined input
			std::cout << "Raw input: " << std::endl << xmlString;
			std::cout << "Streamlined input: \"" << input << "\"" << std::endl;
		}
		
		// Parse the XML declaration, if present
		Declaration* pDecl = parseDeclaration(input, charIndex, positions);
		
		// Parse the XML element tree
		Element* pTree = parseElement(input, charIndex, positions);

		
		// Return the parsed XML document
		return Document(pDecl, pTree);
	}

	/***************************************************************
	* Function: Parser::parseFile()
	* Purpose : Parse an XML file
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	Document Parser::parseFile(const std::string& filePath)
	{
		// Attempt to open the file in binary reading mode
		FILE* pXMLFile = fopen(filePath.c_str(), "rb");

		// Make sure that the XML file was successfully opened
		if (!pXMLFile)
			throw ParseError("Could not open XML file for parsing");

		// Seek to the end of the file
		fseek(pXMLFile, 0, SEEK_END);

		// Get the file size
		size_t fileSize = ftell(pXMLFile);

		// Seek back to the start of the file
		fseek(pXMLFile, 0, SEEK_SET);

		// Allocate a buffer to read the file
		char* pBuffer = new char[fileSize + 1];

		// Read the file into the buffer
		size_t sizeRead = fread(pBuffer, 1, fileSize, pXMLFile);

		// Close the input file
		fclose(pXMLFile);

		// Make sure the file was entirely read
		if (sizeRead != fileSize)
			throw ParseError("Unable to read entire file");

		// Append a null character
		pBuffer[fileSize] = '\0';

		// Store the input in a string object
		std::string rawInput = pBuffer;

		// Delete the temporary buffer
		delete [] pBuffer;

		// Parse the XML input string
		return parseString(rawInput);
	}

	/***************************************************************
	* Function: Parser::saveFile()
	* Purpose : Save an XML tree to a file
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	void Parser::saveFile(const std::string& filePath, const Node* pXMLTree)
	{
		// Attempt to open the file in binary writing mode
		FILE* pXMLFile = fopen(filePath.c_str(), "wb");

		// Make sure that the XML file was successfully opened
		if (!pXMLFile)
			throw ParseError("Could not open file for output");

		// Produce an XML representation of the XML tree
		std::string xmlString = pXMLTree->toString(true, 0);

		// Write the string representation to the output file
		size_t result = fwrite(xmlString.c_str(), xmlString.length(), 1, pXMLFile);
		
		// Close the output file
		fclose(pXMLFile);
		
		// If the write operation failed, throw an exception
		if (result != 1)
			throw ParseError("Could not write to output file");
	}
	
	/***************************************************************
	* Function: Parser::streamline()
	* Purpose : Streamline an XML code string
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	std::string Parser::streamline(const std::string& rawInput, PosVector& positions)
	{
		// Declare a string for the streamlined output
		std::string output;

		// reserve space in the string
		output.reserve(rawInput.length());
		
		// Declare characters for stream processing
		unsigned char lastChar = '\0';
		unsigned char thisChar = '\0';
		unsigned char nextChar = '\0';
		
		// Variable to tell if we are in a CDATA region
		bool inCDATA = false;
		
		// Variable to tell if we are in a comment
		bool inComment = false;
		
		// Variable to indicate the current line number
		size_t lineNumber = 1;

		// Variable to indicate the start of the current line
		size_t lineStart = 0;
		
		// For each character of the input
		for (size_t charIndex = 0; charIndex < rawInput.length(); ++charIndex)
		{
			// Read this character and the next
			thisChar = rawInput[charIndex];
			nextChar = rawInput[charIndex + 1];

			// Construct the text position of the current character
			TextPos charPos(lineNumber, charIndex - lineStart + 1);
			
			// If this character is a newline
			if (thisChar == '\n')
			{
				// Increment the line number
				++lineNumber;
				
				// Store the line start index
				lineStart = charIndex + 1;
			}
			
			// If this is the start of a comment
			if (tokenMatch(rawInput, charIndex, "<!--") && !inCDATA && !inComment)
			{
				// We are now in a comment
				inComment = true;
				
				// Move past the comment opening
				charIndex += 3;
				continue;
			}
			
			// If this is the end of a comment
			if (tokenMatch(rawInput, charIndex, "-->") && inComment)
			{
				// We are no longer in a comment
				inComment = false;
				
				// Move past the comment closing
				charIndex += 2;
				continue;
			}			

			// If this is the start of a CDATA region
			if (tokenMatch(rawInput, charIndex, "<![CDATA[") && !inCDATA && !inComment)
			{
				// We are now in a CDATA region
				inCDATA = true;
			}
			
			// If this is the end of a CDATA region
			if (tokenMatch(rawInput, charIndex, "]]>") && inCDATA)
			{
				// We are no longer in a CDATA region
				inCDATA = false;
			}
			
			// If we are inside a comment
			if (inComment)
			{
				// Skip this character
				continue;
			}			
			
			// If we are not in a CDATA region
			if (!inCDATA)
			{
				// If this character is whitespace
				if (isspace(thisChar))
				{
					// Consider it a space
					thisChar = ' ';
				}
	
				// If the next character is whitespace
				if (isspace(nextChar))
				{
					// Consider it a space
					nextChar = ' ';
				}
	
				// If this character is whitespace
				if (thisChar == ' ')
				{
					// If the next char is whitespace, 
					// or the last was an opening delimiter or a space,
					// or the next is a closing delimiter
					if (nextChar == ' ' || 
						lastChar == '<' || lastChar == '>' || lastChar == ' ' ||
						nextChar == '<' || nextChar == '>' || nextChar == '\0')
					{
						// Ignore this space
						continue;
					}
				}
			}

			// Add this character to the stream
			output += thisChar;

			// Store the last character added
			lastChar = thisChar;
			
			// Store the text position of this character
			positions.push_back(charPos);
		}

		// Add a final text position for the string terminator
		positions.push_back(TextPos(lineNumber, rawInput.length() - lineStart + 1));
		
		// Ensure that there is a position for each character in the output
		assert (output.length() + 1 == positions.size());
		
		// Return the streamlined XML stream
		return output;
	}
	
	/***************************************************************
	* Function: Parser::parseEscapeSeq()
	* Purpose : Streamline an XML code string
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	char Parser::parseEscapeSeq(const std::string& xmlString, size_t& charIndex, const PosVector& positions)
	{
		// If the first character is not the beggining of an escape sequence
		if (xmlString[charIndex] != '&')
		{
			// Throw an exception
			throw ParseError("Invalid escape sequence", positions[charIndex]);
		}

		// Declare an escape sequence string
		std::string escapeSeq;

		// For each character
		for (++charIndex;; ++charIndex)
		{
			// If we are past the length of the input stream
			if (charIndex > xmlString.length())
			{
				// Throw an exception
				throw ParseError("Unexpected end of stream in escape sequence", positions[charIndex]);
			}

			// Extract the current character
			unsigned char thisChar = xmlString[charIndex];

			// If this character is the end of the escape sequence
			if (thisChar == ';')
			{
				// Break out of this loop
				break;
			}

			// If this character is not alphanumeric
			if (!isalnum(thisChar))
			{
				// Throw an exception
				throw ParseError("Non alphanumeric character in escape sequence", positions[charIndex]);
			}

			// Add this character to the escape sequence
			escapeSeq += thisChar;
		}

		// Declare a variable for the escaped character
		char escapedChar;

		// If the escape sequence is empty
		if (escapeSeq == "")
		{
			// Throw an exception
			throw ParseError("Empty escape sequence", positions[charIndex]);
		}

		// Handle known escape sequences
		else if (escapeSeq == "amp")	escapedChar = '&';
		else if (escapeSeq == "lt")		escapedChar = '<';
		else if (escapeSeq == "gt")		escapedChar = '>';
		else if (escapeSeq == "quot")	escapedChar = '\"';

		// Otherwise, the escape sequence is unknown
		else
		{
			// Throw an exception
			throw ParseError("Unknown escape sequence: " + escapeSeq, positions[charIndex]);
		}

		// Return the escaped character
		return escapedChar;
	}

	/***************************************************************
	* Function: Parser::parseTagName()
	* Purpose : Parse the name of an XML tag
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	std::string Parser::parseTagName(const std::string& xmlString, size_t& charIndex, const PosVector& positions)
	{
		// If the first character is '/'
		if (xmlString[charIndex] == '/')
		{
			// Move to the next character
			++charIndex;
		}

		// If the first character is not alphanumeric
		if (!isalnum(xmlString[charIndex]))
		{
			// Throw an exception
			throw ParseError("Invalid tag name", positions[charIndex]);
		}

		// Declare a string for the tag name
		std::string tagName;

		// For each character
		for (;; ++charIndex)
		{
			// If we are past the length of the input stream
			if (charIndex > xmlString.length())
			{
				// Throw an exception
				throw ParseError("Unexpected end of stream in tag name", positions[charIndex]);
			}

			// Extract the current and next characters
			unsigned char thisChar = xmlString[charIndex];
			unsigned char nextChar = xmlString[charIndex+1];

			// If this character is not alphanumeric
			if (!isalnum(thisChar))
			{
				// Throw an exception
				throw ParseError("Non alphanumeric character in tag name (" + charToHex(thisChar) + ")", positions[charIndex]);
			}

			// Add this character to the tag name
			tagName += thisChar;

			// If the next character is a space, a slash or a tag end
			if (isspace(nextChar) || nextChar == '/' || nextChar == '>')
			{
				// This is the end of the name
				break;
			}
		}

		// Return the tag name
		return tagName;
	}	
	
	/***************************************************************
	* Function: Parser::parseAttribute()
	* Purpose : Parse an XML attribute-value pairs
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	std::pair<std::string, std::string> Parser::parseAttribute(const std::string& xmlString, size_t& charIndex, const PosVector& positions)
	{		
		// If the first character is not alphanumeric
		if (!isalnum(xmlString[charIndex]))
		{
			// Throw an exception
			throw ParseError("Invalid attribute name", positions[charIndex]);
		}

		// Declare an attribute name string
		std::string attribName;

		// For each character
		for (;; ++charIndex)
		{
			// If we are past the length of the input stream
			if (charIndex > xmlString.length())
			{
				// Throw an exception
				throw ParseError("Unexpected end of stream in attribute name", positions[charIndex]);
			}

			// Extract the current character
			char thisChar = xmlString[charIndex];

			// If this character is a space or an equal sign
			if (isspace(thisChar) || thisChar == '=')
			{
				// This is the end of the name
				break;
			}

			// If this character is not alphanumeric
			if (!isalnum(thisChar))
			{
				// Throw an exception
				throw ParseError("Non alphanumeric character in attribute name", positions[charIndex]);
			}

			// Add this character to the attribute name
			attribName += thisChar;
		}

		// For each character
		for (;; ++charIndex)
		{
			// If we are past the length of the input stream
			if (charIndex > xmlString.length())
			{
				// Throw an exception
				throw ParseError("Unexpected end of stream in attribute", positions[charIndex]);
			}

			// Extract the current character
			char thisChar = xmlString[charIndex];

			// If this character is an equal sign
			if (thisChar == '=')
			{
				// Move to the next character
				++charIndex;

				// End this loop
				break;
			}

			// If this character is not a space
			if (!isspace(thisChar))
			{
				// Throw an exception
				throw ParseError("Invalid character in attribute", positions[charIndex]);
			}
		}

		// For each character
		for (;; ++charIndex)
		{
			// If we are past the length of the input stream
			if (charIndex > xmlString.length())
			{
				// Throw an exception
				throw ParseError("Unexpected end of stream in attribute", positions[charIndex]);
			}

			// Extract the current character
			char thisChar = xmlString[charIndex];

			// If this character is an opening quote
			if (thisChar == '"')
			{
				// Move to the next character
				++charIndex;

				// End this loop
				break;
			}

			// If this character is not a space
			if (!isspace(thisChar))
			{
				// Throw an exception
				throw ParseError("Invalid character in attribute", positions[charIndex]);
			}
		}

		// Declare an attribute value string
		std::string attribValue;

		// For each character
		for (;; ++charIndex)
		{
			// If we are past the length of the input stream
			if (charIndex > xmlString.length())
			{
				// Throw an exception
				throw ParseError("Unexpected end of stream in attribute value", positions[charIndex]);
			}

			// Extract the current character
			char thisChar = xmlString[charIndex];

			// If this character is the beggining of an escape sequence
			if (thisChar == '&')
			{
				// Parse the escape sequence
				attribValue += parseEscapeSeq(xmlString, charIndex, positions);

				// Move on to the next character
				continue;
			}

			// If this character is a closing quote
			if (thisChar == '"')
			{
				// This is the end of the name
				break;
			}

			// Add this character to the attribute value
			attribValue += thisChar;
		}

		// Return a pair containing the attribute name and value
		return std::pair<std::string, std::string>(attribName, attribValue);
	}
	
	/***************************************************************
	* Function: Parser::parseDeclaration()
	* Purpose : Parse an XML declaration node
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	Declaration* Parser::parseDeclaration(const std::string& xmlString, size_t& charIndex, const PosVector& positions)
	{
		// If this is not an XML declaration node
		if (!tokenMatch(xmlString, charIndex, "<?xml"))
		{
			// Return nothing without attempting to parse
			return NULL;
		}

		// Move past the declaration opening
		charIndex += 5;
		
		// Declare variables for the current and next characters
		unsigned char thisChar;
		unsigned char nextChar;

		// Declare a map of attributes
		std::map<std::string, std::string> attributes;

		// For each character
		for (;; ++charIndex)
		{
			// If we are past the length of the input stream
			if (charIndex > xmlString.length())
			{
				// Throw an exception
				throw ParseError("Unexpected end of stream inside opening tag", positions[charIndex]);
			}

			// Update the current and next characters
			thisChar = xmlString[charIndex];
			nextChar = xmlString[charIndex + 1];

			// If this is a space
			if (isspace(thisChar))
			{
				// Skip the space
				continue;
			}

			// If this is the end of the opening tag
			if (thisChar == '?' && nextChar == '>')
			{
				// Move past the declaration closing
				charIndex += 2;

				// End this loop
				break;
			}

			// If this character is alphanumeric
			if (isalnum(thisChar))
			{
				// Parse this attribute
				std::pair<std::string, std::string> attribute = parseAttribute(xmlString, charIndex, positions);

				// If another attribute with this name was already parsed
				if (attributes.find(attribute.first) != attributes.end())
				{
					// Throw an exception
					throw ParseError("Duplicate attribute name: " + attribute.first, positions[charIndex]);
				}
				
				// Add this attribute to the map
				attributes[attribute.first] = attribute.second;

				// Move to the next character
				continue;
			}

			// If this character was not recognized, throw an exception
			throw ParseError("Invalid character inside opening tag", positions[charIndex]);
		}

		// Create and return a new XML declaration node with the parsed information
		return new Declaration(attributes);
	}
	
	/***************************************************************
	* Function: Parser::parseNode()
	* Purpose : Parse an XML data node
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	Node* Parser::parseNode(const std::string& xmlString, size_t& charIndex, const PosVector& positions)
	{
		// If this is the beginning of a CDATA region
		if (tokenMatch(xmlString, charIndex, "<![CDATA["))
		{
			// Parse the CDATA region
			return parseRawData(xmlString, charIndex, positions);
		}
		
		// If this is the beginning of an XML element
		else if (tokenMatch(xmlString, charIndex, "<"))
		{
			// Parse the XML element recursively
			return parseElement(xmlString, charIndex, positions);
		}
		
		// Otherwise, if this is a text region
		else
		{			
			// Parse the text
			return parseText(xmlString, charIndex, positions);
		}
	}
	
	/***************************************************************
	* Function: Parser::parseElement()
	* Purpose : Parse an XML element
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	Element* Parser::parseElement(const std::string& xmlString, size_t& charIndex, const PosVector& positions)
	{
		// If this XML element does not begin appropriately
		if (xmlString[charIndex] != '<')
		{
			// Throw an exception
			throw ParseError("Invalid XML element opening", positions[charIndex]);
		}

		// Get the current text position
		const TextPos& textPos = positions[charIndex];
		
		// Move to the next character
		++charIndex;

		// Declare variables for the current and next characters
		unsigned char thisChar;
		unsigned char nextChar;

		// Declare a map of attributes
		std::map<std::string, std::string> attributes;

		// Declare a vector of children nodes
		std::vector<Node*> children;

		// Declare a boolean to indicate if this is a leaf tag
		bool isLeaf = false;

		// Parse the tag name
		std::string name = parseTagName(xmlString, charIndex, positions);

		// Move to the next character
		++charIndex;

		// For each character
		for (;; ++charIndex)
		{
			// If we are past the length of the input stream
			if (charIndex > xmlString.length())
			{
				// Throw an exception
				throw ParseError("Unexpected end of stream inside opening tag", positions[charIndex]);
			}

			// Update the current and next characters
			thisChar = xmlString[charIndex];
			nextChar = xmlString[charIndex + 1];

			// If this is a space
			if (isspace(thisChar))
			{
				// Skip the space
				continue;
			}

			// If this is the end of the opening tag
			if (thisChar == '>')
			{
				// Move to the next character
				++charIndex;

				// End this loop
				break;
			}

			// If this character is a leaf tag
			if (thisChar == '/' && nextChar == '>')
			{
				// Set the leaf tag flag
				isLeaf = true;

				// Skip this character
				++charIndex;

				// End this loop
				break;
			}

			// If this character is alphanumeric
			if (isalnum(thisChar))
			{
				// Parse this attribute
				std::pair<std::string, std::string> attribute = parseAttribute(xmlString, charIndex, positions);

				// If another attribute with this name was already parsed
				if (attributes.find(attribute.first) != attributes.end())
				{
					// Throw an exception
					throw ParseError("Duplicate attribute name: " + attribute.first, positions[charIndex]);
				}
				
				// Add this attribute to the map
				attributes[attribute.first] = attribute.second;

				// Move to the next character
				continue;
			}

			// If this character was not recognized, throw an exception
			throw ParseError("Invalid character inside opening tag", positions[charIndex]);
		}

		// If this is not a leaf tag
		if (!isLeaf)
		{
			// For each character
			for (;; ++charIndex)
			{
				// If we are past the length of the input stream
				if (charIndex > xmlString.length())
				{
					// Throw an exception
					throw ParseError("Unexpected end of stream inside tag", positions[charIndex]);
				}

				// Update the current and next characters
				thisChar = xmlString[charIndex];
				nextChar = xmlString[charIndex + 1];

				// If this is a space
				if (isspace(thisChar))
				{
					// Skip the space
					continue;
				}

				// If this is the beggining of the closing tag
				if (thisChar == '<' && nextChar == '/')
				{
					// Skip these characters
					charIndex += 2;

					// Break out of this loop
					break;
				}

				// Parse the child node at this point
				Node* pChildNode = parseNode(xmlString, charIndex, positions);

				// Add the child node to the list
				children.push_back(pChildNode);
			}

			// Parse the closing tag name
			std::string closingName = parseTagName(xmlString, charIndex, positions);

			// If the tag names do not match
			if (name != closingName)
			{
				// Throw an exception
				throw ParseError("Unmatching closing tag for \"" + name + "\" : \"/" + closingName + "\"", positions[charIndex]);
			}

			// Move to the next character
			++charIndex;

			// If the closing tag does not end properly
			if (xmlString[charIndex] != '>')
			{
				// Throw an exception
				throw ParseError("Malformed closing tag", positions[charIndex]);
			}
		}

		// Create and return a new XML element with the parsed information
		return new Element(name, attributes, children, textPos, isLeaf);
	}
	
	/***************************************************************
	* Function: Parser::parseRawData()
	* Purpose : Parse an XML CDATA region
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	RawData* Parser::parseRawData(const std::string& xmlString, size_t& charIndex, const PosVector& positions)
	{
		// If the CDATA region does not begin appropriately
		if (!tokenMatch(xmlString, charIndex, "<![CDATA["))
		{
			// Throw an exception
			throw ParseError("Invalid CDATA region opening", positions[charIndex]);
		}
		
		// Move past the CDATA opening
		charIndex += 9;
		
		// Declare a string to store the region contents
		std::string contents;
		
		// For each character
		for (;; ++charIndex)
		{
			// If we are past the length of the input stream
			if (charIndex > xmlString.length())
			{
				// Throw an exception
				throw ParseError("Unexpected end of stream inside CDATA region", positions[charIndex]);
			}

			// Update the current character
			char thisChar = xmlString[charIndex];
			
			// If this is the closing of the CDATA region
			if (tokenMatch(xmlString, charIndex, "]]>"))
			{
				// Move past the CDATA closing
				charIndex += 2;
				
				// Break out of this loop
				break;
			}
			
			// Add the character to the region contents
			contents.push_back(thisChar);
		}
		
		// Create and return a new raw data node
		return new RawData(contents);
	}
	
	/***************************************************************
	* Function: Parser::parseText()
	* Purpose : Parse an XML text region
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	Text* Parser::parseText(const std::string& xmlString, size_t& charIndex, const PosVector& positions)
	{		
		// Declare a string to store the text
		std::string text;
		
		// For each character
		for (;; ++charIndex)
		{
			// If we are past the length of the input stream
			if (charIndex >= xmlString.length())
			{
				// Throw an exception
				throw ParseError("Unexpected end of stream in text region", positions[charIndex]);
			}

			// Extract the current character
			char thisChar = xmlString[charIndex];

			// If this character is the beggining of an escape sequence
			if (thisChar == '&')
			{
				// Parse the escape sequence
				text += parseEscapeSeq(xmlString, charIndex, positions);

				// Move on to the next character
				continue;
			}

			// If this character is the beginning of a tag
			if (thisChar == '<')
			{
				// Move back one character
				charIndex--;
				
				// Break out of this loop
				break;
			}

			// Add this character to the text
			text += thisChar;
		}	
		
		// Create and returna new text node
		return new Text(text);
	}
	
	/***************************************************************
	* Function: escapeString()
	* Purpose : Escape an XML text string for output
	* Initial : Maxime Chevalier-Boisvert on May 28, 2006
	****************************************************************
	Revisions and bug fixes:
	*/
	std::string escapeString(const std::string& input)
	{
		// Declare a variable for the escaped string
		std::string escapedStr;

		// For each character of the input string
		for (size_t index = 0; index < input.length(); ++index)
		{
			// Extract this character
			char thisChar = input[index];

			// Switch on the current character
			switch (thisChar)
			{
				// Handle known escape sequences
				case '&':	escapedStr += "&amp;";	break;
				case '<':	escapedStr += "&lt;";	break;
				case '>':	escapedStr += "&gt;";	break;
				case '\"':	escapedStr += "&quot;";	break;
				case '\'':	escapedStr += "&apos;";	break;

				// By defaul, the character does not need to be escaped
				default: escapedStr += thisChar;
			}
		}

		// Return the escaped string
		return escapedStr;
	}
}
