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

#ifndef CLIENTSOCKET_H_
#define CLIENTSOCKET_H_

#include <string>
#include <unistd.h>
#include "sock.h"

/***************************************************************
* Class   : ClientSocket
* Purpose : Represent a TCP client socket
* Initial : Nurudeen A. Lameed on April 28, 2009
****************************************************************
Revisions and bug fixes:
*/

class ClientSocket
{
public:
	ClientSocket();
	// constructor, creates a socket and binds it to localPortNo
	ClientSocket(const int localPortNo);

	// constructor, creates a socket and connect it to the server process with (svrname, port)
	ClientSocket(const char* svrname, const int svrportNo);

	// create a socket for communication with another socket by a server
	ClientSocket(const Socket sockfd, struct sockaddr_in& sockAddr);

	// destructor to free up resources.
	virtual ~ClientSocket();

	// returns port number of the socket
	int getLocalPortNo() const { return portNo; }

	// returns port number of the socket
	int getRemotePortNo() const { return serverPortNo; }

	// send all the data in buf
	bool sendAll(const char* buf, const int len);

	// receive all data into buf
	int receiveAll();
	
	// host of this socket
	const char* getHost() const { return serverName; }

	// connect a socket to a server
	void connectSocket(const char*, const int);

	// return the address of the server
	const char* getRemoteAddress() const;

	// check whether a socket is connected to a server
	bool isConnected() const { return isConn; }

	// close the socket
	void closeSocket() const { close(socketFD); }

	// must call startUP, for the first socket of this process
	static void startUP() {_WSA_STARTUP(); }

	// for clean up
	static void cleanUP() {_WSA_CLEANUP(); }

	// continuously receive data from a socket until a null char is received;
	// must receive a null character to stop reading; buffers the characters
	// read into the buffer and appearing after the null character.
	std::string bufferedReceiveUntilNull();

	// continuously receive data from a socket until a null char is received;
	// must receive a null character to stop reading
	std::string receiveUntilNull();

	// checks whether a name is an IP address.
	static bool isIPAddress(const char*);

	// checks whether a port is bound to a socket
	static bool isBound(const int portNo);


private:
	// helper function for creating a socket
	void create();

	// helper function for connecting a socket
	void connectSocket();

	// helper function for binding a socket to a portNo
	void bindSocket();

	// maximum buffer size
	static const int MAX_BUFFER_SIZE = 1024;

	// internal number (socket file descriptor)
	Socket socketFD;

	// port number of the local host
	int portNo;

	// host name of the socket
	const char* serverName;

	// port number of the remote host
	int serverPortNo;

	// address info of this host
	struct sockaddr_in socketAddr;

	// address info of the server
	struct sockaddr_in serverAddr;

	// updates connection state
	bool isConn;

	// message buffer
	char buf[MAX_BUFFER_SIZE];

	// current buffer char pointer
	int pos;

	// one position past the end of the character buffer
	int endPos;
};

/***************************************************************
* Class   : ConnectionError
* Purpose : Represent a connection error exception
* Initial : Nurudeen A. Lameed on May 7, 2009
****************************************************************
Revisions and bug fixes:
*/
class ConnectionError : public std::runtime_error {
  public:
	// constructor, errMsg holds the error message
	ConnectionError(std::string errMsg):runtime_error(errMsg){}

	virtual ~ConnectionError() throw () {}
};

/***************************************************************
* Class   : SocketIOError
* Purpose : Represent an input/output error
* Initial : Nurudeen A. Lameed on May 20, 2009
****************************************************************
Revisions and bug fixes:
*/
class SocketIOError : public std::runtime_error {
  public:
	// constructor, errMsg holds the error message
	SocketIOError(std::string errMsg):runtime_error(errMsg){}

	virtual ~SocketIOError() throw () {}
};

#endif /* CLIENTSOCKET_H_ */
