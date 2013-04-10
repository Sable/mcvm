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
#ifndef XML_H_
#define XML_H_

// Header files
#include <string>
#include <vector>
#include <map>

// Include this in the XML namespace
namespace XML
{
	/***************************************************************
	* Class   : TextPos
	* Purpose : Represent a position in a text file
	* Initial : Maxime Chevalier-Boisvert on October 19, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	class TextPos
	{
	public:
	
		// Constructors
		TextPos(size_t line, size_t col) : m_line(line), m_column(col) {}
		TextPos() : m_line(0), m_column(0) {}
		
		// Method to obtain a string representation
		std::string toString() const;
		
		// Accessor to get the line number
		size_t getLine() const { return m_line; }
		
		// Accessor to get the column number
		size_t getColumn() const { return m_column; }
	
	private:
	
		// File coordinates
		size_t m_line;
		size_t m_column;
	};
	
	// Text position vector definition
	typedef std::vector<TextPos> PosVector;
	
	/***************************************************************
	* Class   : ParseError
	* Purpose : Represent an XML parsing error
	* Initial : Maxime Chevalier-Boisvert on October 19, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	class ParseError
	{
	public:
	
		// Constructor
		ParseError(const std::string& errorText, const TextPos& textPos = TextPos())
		: m_errorText(errorText), m_textPos(textPos) {}
	
		// Method to obtain a string representation
		std::string toString() const;
		
		// Accessor to get error text
		const std::string& getErrorText() const { return m_errorText; }
	
		// Accessor to get the text position
		const TextPos& getTextPos() const { return m_textPos; }
	
	private:
	
		// Error description text
		std::string m_errorText;
	
		// Text position of the error
		TextPos m_textPos;
	};
	
	/***************************************************************
	* Class   : Node
	* Purpose : Represent an XML data node
	* Initial : Maxime Chevalier-Boisvert on October 19, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	class Node
	{
	public:
		
		// Enumerate the node types
		enum Type
		{
			TEXT,
			RAWDATA,
			ELEMENT,
			DECLARATION,
		};
		
		// Constructor and destructor
		Node() {}
		virtual ~Node() {}
		
		// Method to copy this node or subtree
		virtual Node* copy() const = 0;
		
		// Method to generate a string representation
		virtual std::string toString(bool indent = false, size_t level = 0) const = 0;
		
		// Method to get the type of this node
		virtual Type getType() const = 0;
	};
	
	/***************************************************************
	* Class   : Text
	* Purpose : Represent a parsed text string in an XML file
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/	
	class Text : public Node
	{
	public:
		
		// Constructor
		Text(const std::string& text) : m_text(text) {}
		
		// Method to copy this node or subtree
		Node* copy() const { return new Text(m_text); }

		// Method to generate a string representation
		std::string toString(bool indent = false, size_t level = 0) const;
		
		// Method to get the type of this node
		Type getType() const { return TEXT; };
		
		// Accessor to get the text contents
		const std::string getText() const { return m_text; }
		
	private:
	
		// Text contents
		std::string m_text;
	};
	
	/***************************************************************
	* Class   : RawData
	* Purpose : Represent unparsed character data
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	class RawData : public Node
	{
	public:
		
		// Constructor
		RawData(const std::string& contents) : m_contents(contents) {}
		
		// Method to copy this node or subtree
		Node* copy() const { return new RawData(m_contents); }
		
		// Method to generate a string representation
		std::string toString(bool indent = false, size_t level = 0) const;
		
		// Method to get the type of this node
		Type getType() const { return RAWDATA; };

		// Accessor to get the text contents
		const std::string getContents() const { return m_contents; }		
		
	private:
		
		// Raw data contents
		std::string m_contents;
	};	
	
	/***************************************************************
	* Class   : Element
	* Purpose : Represent an XML element
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	class Element : public Node
	{
	public:

		// Constructors and destructor
		Element(
			const std::string& name,
			const std::map<std::string, std::string>& attributes,
			const std::vector<Node*>& children,
			const TextPos& textPos,
			bool leaf
		) : m_name(name), m_attributes(attributes), m_children(children), m_textPos(textPos), m_leaf(leaf) {}
			Element(
			const std::string& name,
			const TextPos& textPos = TextPos(),
			bool leaf = true
		) : m_name(name), m_textPos(textPos), m_leaf(leaf) {}
		Element() : m_leaf(true) {}
		~Element();
		
		// Method to set the value of an attribute
		void setStringAttrib(const std::string& name, const std::string& value);

		// Method to get the string value of an attribute
		const std::string& getStringAttrib(const std::string& name) const;

		// Method to get the floating-point value of an attribute
		float getFloatAttrib(const std::string& name) const;

		// Method to get the integer value of an attribute
		int getIntAttrib(const std::string& name) const;

		// Method to get the boolean value of an attribute
		bool getBoolAttrib(const std::string& name) const;
		
		// Method to get a specific child node
		Node* getChildNode(size_t index) const;
		
		// Method to get a child node known to be an element
		Element* getChildElement(size_t index) const;
		
		// Method to copy this node or subtree
		Node* copy() const;
		
		// Method to generate a string representation
		std::string toString(bool indent = false, size_t level = 0) const;
		
		// Method to get the type of this node
		Type getType() const { return ELEMENT; };
		
		// Accessor to get the tag name
		const std::string& getName() const { return m_name; }
		
		// Accessor to get the attributes
		const std::map<std::string, std::string>& getAttributes() const { return m_attributes; }

		// Accessor to get the number of children nodes
		size_t getNumChildren() const { return m_children.size(); }
		
		// Accessor to get the children nodes
		const std::vector<Node*>& getChildren() const { return m_children; }
		
		// Accessor to get the text position
		const TextPos& getTextPos() const { return m_textPos; }
		
		// Accessor to tell if the tag is a leaf
		bool isLeaf() const { return m_leaf; }
		
	private:

		// XML element name
		std::string m_name;

		// XML element attributes
		std::map<std::string, std::string> m_attributes;

		// Children XML nodes
		std::vector<Node*> m_children;

		// Text position
		TextPos m_textPos;
		
		// Indicates if this is a leaf tag
		bool m_leaf;		
	};
	
	/***************************************************************
	* Class   : Declaration
	* Purpose : Represent an XML format declaration
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	class Declaration : public Node
	{
	public:
		
		// Constructor
		Declaration(const std::map<std::string, std::string>& attributes) : m_attributes(attributes) {}
		
		// Method to copy this node or subtree
		Node* copy() const { return new Declaration(m_attributes); }
		
		// Method to generate a string representation
		std::string toString(bool indent = false, size_t level = 0) const;
		
		// Method to get the type of this node
		Type getType() const { return DECLARATION; };
		
		// Accessor to get the attributes
		const std::map<std::string, std::string>& getAttributes() const { return m_attributes; }
		
	private:
		
		// XML element attributes
		std::map<std::string, std::string> m_attributes;
	};

	/***************************************************************
	* Class   : Document
	* Purpose : Represent a parsed XML document
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/	
	class Document
	{
	public:
		
		// Constructors and destructor
		Document(Declaration* pDecl, Element* pTree) : m_pDecl(pDecl), m_pTree(pTree) {}
		Document() : m_pDecl(NULL), m_pTree(NULL) {}
		Document(const Document& document);
		~Document() { delete m_pDecl; delete m_pTree; }
		
		// Method to generate a string representation
		std::string toString(bool indent = false);
		
		// Assignment operator
		Document& operator = (const Document& document);
		
		// Mutator to set the declaration
		void setDecl(const Declaration* pDecl) { m_pDecl = (Declaration*)pDecl->copy(); }
		
		// Mutator to set the tree root
		void setTree(const Element* pTree) { m_pTree = (Element*)pTree->copy(); }
		
		// Accessor to get the declaration
		const Declaration* getDecl() const { return m_pDecl; }
		
		// Accessor to get the tree root
		const Element* getTree() const { return m_pTree; }
		
	private:
		
		// XML document declaration
		Declaration* m_pDecl;
		
		// XML tree root
		Element* m_pTree;
	};
	
	/***************************************************************
	* Class   : Parser
	* Purpose : Parse XML data into tree form
	* Initial : Maxime Chevalier-Boisvert on October 20, 2008
	****************************************************************
	Revisions and bug fixes:
	*/
	class Parser
	{
	public:
		
		// Method to parse an XML input string
		Document parseString(const std::string& xmlString);

		// Parse an XML file
		Document parseFile(const std::string& filePath);

		// Save an XML file
		void saveFile(const std::string& filePath, const Node* pXMLTree);
		
	private:
		
		// Method to streamline an XML input stream
		std::string streamline(const std::string& rawInput, PosVector& positions);
		
		// Method to parse an XML escape sequence
		char parseEscapeSeq(const std::string& xmlString, size_t& charIndex, const PosVector& positions);

		// Method to parse an XML tag name
		std::string parseTagName(const std::string& xmlString, size_t& charIndex, const PosVector& positions);
		
		// Method to parse XML element attributes
		std::pair<std::string, std::string> parseAttribute(const std::string& xmlString, size_t& charIndex, const PosVector& positions);
		
		// Method to parse the XML declaration node
		Declaration* parseDeclaration(const std::string& xmlString, size_t& charIndex, const PosVector& positions);
		
		// Method to parse an XML data node
		Node* parseNode(const std::string& xmlString, size_t& charIndex, const PosVector& positions);
		
		// Method to parse an XML element
		Element* parseElement(const std::string& xmlString, size_t& charIndex, const PosVector& positions);
		
		// Method to parse an XML CDATA region
		RawData* parseRawData(const std::string& xmlString, size_t& charIndex, const PosVector& positions);
		
		// Method to parse an XML text region
		Text* parseText(const std::string& xmlString, size_t& charIndex, const PosVector& positions);				
	};
	
	// Function to escape an XML string for output
	std::string escapeString(const std::string& input);
}

#endif // #ifndef XML_H_
