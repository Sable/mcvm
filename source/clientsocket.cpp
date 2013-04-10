// =========================================================================== //
//                                                                             //
// Copyright 2009 Nurudeen Abiodun Lameed and McGill University.               //
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
//   limitations under the License.                                            //
//                                                                             //
// =========================================================================== //

// header files
#include <sstream>
#include <iostream>
#include "clientsocket.h"

/***************************************************************
* Function: ClientSocket::ClientSocket()
* Purpose : Default constructor, creates a TCP socket client
* Initial : Nurudeen A. Lameed on April 28, 2009
****************************************************************
Revisions and bug fixes:
*/
ClientSocket::ClientSocket():socketFD(0), portNo(0), serverName(0),
serverPortNo(0), isConn(false), pos(0), endPos(0)
{
	// create socket file descriptor
	create();
}

/***************************************************************************
* Function: ClientSocket::ClientSocket()
* Purpose : A constructor, creates a TCP socket client bound to localPortNo
* Initial : Nurudeen A. Lameed on April 28, 2009
****************************************************************************
Revisions and bug fixes:
*/
ClientSocket::ClientSocket(const int localPortNo):socketFD(0), portNo(localPortNo), serverName(0),
serverPortNo(0), isConn(false), pos(0), endPos(0)
{
	// create socket file descriptor
	create();

	// bind socket to localPortNo
	bindSocket();
}

/***************************************************************************
* Function: ClientSocket::ClientSocket()
* Purpose : A constructor, creates a TCP socket connected to a server
* Initial : Nurudeen A. Lameed on April 28, 2009
****************************************************************************
Revisions and bug fixes:
*/
ClientSocket::ClientSocket(const char* svrName, const int svrPort):socketFD(0), portNo(0),
serverName(svrName), serverPortNo(svrPort), isConn(false), pos(0), endPos(0)
{
	// create socket file descriptor
	create();

	// connect to the server
	connectSocket(svrName, svrPort);
}

/****************************************************************************************
* Function: ClientSocket::ClientSocket()
* Purpose : A constructor, creates a TCP socket connected to a server address by address
* Initial : Nurudeen A. Lameed on April 28, 2009
*****************************************************************************************
Revisions and bug fixes:
*/
ClientSocket::ClientSocket(const Socket sockfd, struct sockaddr_in& sockAddr):
socketFD(sockfd), portNo(ntohs(sockAddr.sin_port)), serverName(0), serverPortNo(0),
socketAddr(sockAddr), isConn(true), pos(0), endPos(0)
{
}

/***************************************************
* Function: ClientSocket::sendAll()
* Purpose : Sends a ll data in the buffer upto len
* Initial : Nurudeen A. Lameed on April 28, 2009
****************************************************
Revisions and bug fixes:
*/
bool ClientSocket::sendAll(const char* bytes, const int len)
{
	// total bytes sent
	int total = 0;

	// no of bytes remaining to be sent
	int bytesLeft = len;

	// no of bytes sent by the communication module
	int bytesSent;

	while ( total < len )
	{
		// send the bytes
		bytesSent = send(socketFD, bytes + total, bytesLeft, 0);

		// an IO error has occured?
		if ( bytesSent == -1)
		{
			perror("send");
			throw SocketIOError("Error sending data");
		}
		// accumulate the total number of bytes sent so far
		total += bytesSent;

		// compute the remaining bytes to be sent
		bytesLeft -= bytesSent;
	}

	return total == len;  // everything sent?
}

/*************************************************************
* Function: ClientSocket::bufferedReceiveUntilNull()
* Purpose : Receives data until a null character is received.
* Initial : Nurudeen A. Lameed on Jun 01, 2009
**************************************************************
Revisions and bug fixes:
*/
std::string  ClientSocket::bufferedReceiveUntilNull()
{
	// terminating condition
	bool nullNotFound = true;

	// accumulate characters in data
	std::string data = "";

	while (nullNotFound)
	{
		if (pos < endPos && buf[pos]) // data available
		{
			// collect them until null
			for(; pos < endPos && buf[pos]; ++pos)
			{
				// append the character
				data += buf[pos];
			}
		}
		else if (pos == endPos)	// empty buffer; refill the buffer
		{
			// read from the socket
			endPos  = recv(socketFD, buf, MAX_BUFFER_SIZE, 0);

			// reset pos
			pos = 0;

			// Partial data? The other peer has closed the connection
			if (endPos == 0)
			{
				isConn = false;
				perror("recv");
				throw ConnectionError("Connection broken");
			}

			// IO error
			if (endPos == -1)
        	{
				perror("recv");
				throw SocketIOError("Error receiving data");
        	}
		}
		else // null character found
		{
			nullNotFound = false;					// we are done
			int e = pos;
			++pos %= endPos;   						// skip the null
			endPos = (endPos == e+1 ? 0: endPos);  	// adjust endPos
		}
	}

	return data;
}

/*************************************************************
* Function: ClientSocket::receiveUntilNull()
* Purpose : Receives data until a null character is received.
* Initial : Nurudeen A. Lameed on April 28, 2009
**************************************************************
Revisions and bug fixes:
*/
std::string  ClientSocket::receiveUntilNull()
{	
	int noOfBytes  = recv(socketFD, buf, MAX_BUFFER_SIZE - 1, 0);

	std::string data = "";

	char* c;
	while (noOfBytes > 0)
	{
		int i = 0;
		for (c = buf; i < noOfBytes && *c; ++c, ++i);
		if ( i >= noOfBytes)
		{
			buf[noOfBytes] = '\0';
			data += std::string(buf);
			noOfBytes  = recv(socketFD, buf, MAX_BUFFER_SIZE - 1 , 0); // try again ...
		}
		else
		{
			data += std::string(buf);
			
			break;
		}
	}
	// Partial data? The other peer has closed the connection
	if (noOfBytes == 0)
	{
		isConn = false;
		throw ConnectionError("Connection broken");
	}

	// any IO errors ?
	if (noOfBytes == -1)
	{
		perror("recv");
		throw SocketIOError("Error receiving data");
   	}
	
	return data;
}

/***************************************************
* Function: ClientSocket::receiveAll()
* Purpose : Receives a maximum of 1023 characters
* Initial : Nurudeen A. Lameed on April 28, 2009
****************************************************
Revisions and bug fixes:
*/
int ClientSocket::receiveAll()
{
	// receive all data; max size = MAX_BUFFER_SIZE - 1
	int noOfBytes = recv(socketFD, buf, MAX_BUFFER_SIZE - 1 , 0);

	// test for errors
	if (noOfBytes == 0)
	{
		isConn = false;
		throw ConnectionError("Connection broken");
	}

	// test for IO errors
	if (noOfBytes == -1)
	{
		perror("recv");
		throw SocketIOError("Error receiving data");
	}

	// append a null character
	buf[noOfBytes] = '\0';

	return noOfBytes;
}

/***************************************************
* Function: ClientSocket::create()
* Purpose : helper function, creates a socket
* Initial : Nurudeen A. Lameed on April 28, 2009
****************************************************
Revisions and bug fixes:
*/
void ClientSocket::create()
{
	// initialise this socket address
	socketAddr.sin_family = AF_INET;
	socketAddr.sin_port =  htons(portNo);
	socketAddr.sin_addr.s_addr = INADDR_ANY;
	memset(&(socketAddr.sin_zero), '\0', 8);

	// create a TCP socket
	socketFD = socket(PF_INET, SOCK_STREAM, 0);

	// any errors?
	if ( socketFD == -1 )
	{
		perror("socket");
		throw ConnectionError("Cannot create socket");
	}
}

/***************************************************
* Function: ClientSocket::connectSocket()
* Purpose : Connects this socket to a server
* Initial : Nurudeen A. Lameed on April 28, 2009
****************************************************
Revisions and bug fixes:
*/
void ClientSocket::connectSocket(const char* svrName, const int svrPort)
{
	// update server info: server name
	serverName = svrName;

	// update server info: server port number
	serverPortNo = svrPort;

	//initialise server socket address
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port =  htons(svrPort);
	struct hostent* he;
    const char * me;

    // svrName is an IP address?
    if ( isIPAddress(svrName) )
    {
    	struct in_addr addr;
    	inet_aton(svrName, &addr);
    	he = gethostbyaddr(&addr, sizeof(struct in_addr), AF_INET);
    	me = "gethostbyaddr";
    }
    else
    {
    	he = gethostbyname(svrName);
    	me = "gethostbyname";
    }

    // any errors?
    if (!he)
    {
    	herror(me);
    	throw ConnectionError("Connect(): Unknown host ");
    }

    // set the remaining component of the server socket address
	serverAddr.sin_addr = *((struct in_addr*)he->h_addr);
	memset(&(serverAddr.sin_zero), '\0', 8);

	// attempt to connect
	connectSocket();
}

/*************************************************************
* Function: ClientSocket::connectSocket()
* Purpose : helper function; connects this socket to a server
* Initial : Nurudeen A. Lameed on April 28, 2009
**************************************************************
Revisions and bug fixes:
*/
void ClientSocket::connectSocket()
{
	if (connect(socketFD, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr)) ==  -1)
	{
		// get the current error number
		int err = errno;

		// form an output string
		std::ostringstream sout;
		sout << "connectSocket(): " << strerror(err) << "; Error connecting to "
			 <<  serverName << ":" << serverPortNo << "\n";

		// failure, reset connection flag
		isConn = false;

		// throw an error
		throw ConnectionError(sout.str());
	}

	// success, reset connection flag
	isConn = true;
}

/*************************************************************
* Function: ClientSocket::bindSocket()
* Purpose : Binds this socket to port a number
* Initial : Nurudeen A. Lameed on April 28, 2009
**************************************************************
Revisions and bug fixes:
*/
void ClientSocket::bindSocket()
{
	if (bind(socketFD, (struct sockaddr*)&socketAddr, sizeof(struct sockaddr)) == -1)
	{
		perror("bind");
		throw ConnectionError("Error binding socket to port");
	}
}

/*************************************************************
* Function: ClientSocket::getRemoteAddress()
* Purpose : Returns the address of the server.
* Initial : Nurudeen A. Lameed on April 28, 2009
**************************************************************
Revisions and bug fixes:
*/
const char* ClientSocket::getRemoteAddress() const
{
	return inet_ntoa(serverAddr.sin_addr);
}

/*************************************************************
* Function: ClientSocket::isIPAddress()
* Purpose : Checks whether a name is an ip address (v1PV4)
* Initial : Nurudeen A. Lameed on April 28, 2009
**************************************************************
Revisions and bug fixes:
*/
bool ClientSocket::isIPAddress(const char* id)
{
	for(const char *c = id; *c; ++c)
	{
		if (!(isdigit(*c) || *c == '.'))
			return false;
	}

	return true;
}

/*************************************************************
* Function: ClientSocket::isBound()
* Purpose : Checks whether a port is bound to a socket
* Initial : Nurudeen A. Lameed on May 18, 2009
**************************************************************
Revisions and bug fixes:
*/
bool ClientSocket::isBound(const int pNo)
{
	struct sockaddr_in newsocketAddr;

	// intialize newsocketAddr
	newsocketAddr.sin_family = AF_INET;
	newsocketAddr.sin_port =  htons(pNo);
	newsocketAddr.sin_addr.s_addr = INADDR_ANY;
	memset(&(newsocketAddr.sin_zero), '\0', 8);

	int newsockfd;

	// create a new socket file descriptor
	if ((newsockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		throw ConnectionError("Cannot create socket");
	}

	// set this socket options to reuse the host address
	if (setsockopt(newsockfd, SOL_SOCKET, SO_REUSEADDR, "1",sizeof(int)) == -1)
	{
		close(newsockfd);
		perror("setsockopt");
		throw ConnectionError("Cannot set socket options");
	}

	// try to bind this socket
	if (bind(newsockfd, (struct sockaddr*)&newsocketAddr, sizeof(struct sockaddr)) == -1)
	{
		close(newsockfd);
		return true;
	}

	// try to listen on this socket
	if ( listen(newsockfd, 1) == -1)
	{
		close(newsockfd);
		return true;
	}

	close(newsockfd);

	return false;
}

/*************************************************************
* Function: ClientSocket::~ClientSocket()
* Purpose : Destructor
* Initial : Nurudeen A. Lameed on April 28, 2009
**************************************************************
Revisions and bug fixes:
*/
ClientSocket::~ClientSocket()
{
	closeSocket();
}
