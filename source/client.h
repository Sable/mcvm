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

#ifndef CLIENT_H_
#define CLIENT_H_

#include <string>
#include <cstdio>
#include <pthread.h>
#include "clientsocket.h"

/*******************************************************************
* Class   : Client
* Purpose : Represents command line options to natlab; starts natlab
* Initial : Nurudeen A. Lameed on May 5, 2009
********************************************************************
Revisions and bug fixes:
*/
class Client
{
public:
	// default port to the frontend
	static const int FRONTEND_DEFAULT_PORT = 47146;

	// default host name, "localhost"
	static const char* FRONTEND_DEFAULT_HOST;

	~Client();

	// opens a stream to the frontend to mcvm
	static void openSocketStream(const char *svrName, const int svrPortNo);

	// send parsefile command to the frontend
	static std::string parseFile(const std::string& filePath);

	// sends a parsetext command.
	static std::string parseText(const std::string& txt);

	// sends a shutdown command to (natlab).
	static std::string shutdown();

	// closes and releases stream resources used.
	static void closeSocketStream();

	// start natlab on a given port
	static void start(const char* svrName, const int svrPortNo);

	// connect to natlab
	static void connect();
	
private:
	// private constructor, must not be called.
	Client();

	// sends a  command to (natlab).
	static std::string sendCommand(const char* command);
	
	// wait for the heartbeat thread to terminate
	inline static void waitHBThread();
		
	// create a thread for sending heartbeat messages
	inline static int createHBThread();
	
	// generate and send hearbeat messages 
	static void* heartbeat(void* arg);
	
	// test whether a port number is free, return a free port number
	static void validatePortNo(const int svrPortNo);
	
	// Compiler front-end smallest unregistered port number 
	static const int PORT_NO_LOWER_BOUND = 49152;
	
	// Compiler front-end no of unregistered port numbers
	static const int PORT_NO_POOL_SIZE = 16383;

	// Compiler front-end command-line entry point
	static const std::string FRONTEND_ENTRY_POINT;

	// Compiler front-end command arguments
	static const std::string FRONTEND_ARGUMENTS;

	// Java virtual machine arguments for running the front-end
	static const std::string FRONTEND_JVM_ARGUMENTS;

	// Java virtual machine base command
	static const std::string FRONTEND_JVM;

	// maximum number of seconds to wait for the server to start.
	static const int MAX_ELAPSED_TIME = 5;

	// socket stream pointer.
	static ClientSocket* socketStream;
	
	// heartbeat rate (inter-hearbeat message)
	static const int MAX_INTERVAL = 2;
	
	// mutual exclusion object
	static pthread_mutex_t mutex;
	
	// heartbeat thread id
	static pthread_t hb;
	
	// the port number attached to the frontend	
	static int serverPortNo;
		
	// the name of the server running the frontend
	static const char* serverName;
};

#endif /* CLIENT_H_ */


