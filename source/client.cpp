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

#include <stdexcept>
#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>
#include <ctime>
#include "client.h"
#include "filesystem.h"
#include "xml.h"

// communication interface to the server(natlab frontend)
ClientSocket* Client::socketStream = 0;

// the port number attached to the frontend	
int Client::serverPortNo = Client::FRONTEND_DEFAULT_PORT;
		
// default hostname of the server.
const char * Client::FRONTEND_DEFAULT_HOST =  "localhost";

// the name of the server running the frontend
const char* Client::serverName = Client::FRONTEND_DEFAULT_HOST;

// Compiler front-end command-line entry point
const std::string Client::FRONTEND_ENTRY_POINT = "natlab.sh";

// Compiler front-end command arguments
const std::string Client::FRONTEND_ARGUMENTS =  " -matlab -xml -quiet -server &";

// mutual exclusion object
pthread_mutex_t Client::mutex = PTHREAD_MUTEX_INITIALIZER;

// initialize heartbeat thread id
pthread_t Client::hb = 0;

/*******************************************************************
* Function: Client::Client()
* Purpose : Default constructor
* Initial : Nurudeen A. Lameed on May 5, 2009
********************************************************************
Revisions and bug fixes:
*/
Client::Client(){}

/*******************************************************************
* Function: Client::~Client()
* Purpose : Destructor
* Initial : Nurudeen A. Lameed on May 5, 2009
********************************************************************
Revisions and bug fixes:
*/
Client::~Client() {}

/*******************************************************************
* Function: Client::start()
* Purpose : Start Natlab without establishing a connection
* Initial : Nurudeen A. Lameed on May 15, 2009
********************************************************************
Revisions and bug fixes:
*/
void Client::start(const char* svrName, const int svrPortNo)
{
	// set the name of the server
	serverName = svrName;
	
	// test whether the port is free
	validatePortNo(svrPortNo);
	
	// open a string stream
	std::ostringstream command;
	
	// form the command to start natlab in server mode
	command << FRONTEND_ENTRY_POINT	<< " -sp " << serverPortNo << " " << FRONTEND_ARGUMENTS;
	
	// start the service as a background process.
	system(command.str().c_str());

	// create a client socket
	if (!socketStream)
	{
		ClientSocket::startUP();
		socketStream = new ClientSocket();
	}
}

/*******************************************************************
* Function: Client::validatePortNo
* Purpose : Get a free port number
* Initial : Nurudeen A. Lameed on June 17, 2009
********************************************************************
Revisions and bug fixes:
*/
void Client::validatePortNo(const int svrPortNo)
{
	// copy server port no
	int portNo = svrPortNo;
	
	// seed the pseudo random number generator
	srand( time(0) );
	
	// count the number of attempts
	int tries = 0;
	
	while (ClientSocket::isBound(portNo))
	{
		// pick a port randomly from the pool of unregistered ports (starting from PORT_NO_LOWER_BOUND) 
		portNo = PORT_NO_LOWER_BOUND + rand() % PORT_NO_POOL_SIZE;
		
		// a safe guard against infinite loop
		if ( ++tries > PORT_NO_POOL_SIZE )
		{
			throw ConnectionError("Unable to obtain a free port; pls. try again");
		}
	}
	
	// store the selected port 
	serverPortNo = portNo;
}

/*******************************************************************
* Function: Client::connect()
* Purpose : opens a connection to natlab
* Initial : Nurudeen A. Lameed on May 15, 2009
********************************************************************
Revisions and bug fixes:
*/
void Client::connect()
{
	if (!serverName)
	{
		throw ConnectionError("Server name cannot be null");
	}
	
	// count the number of secs this process has waited trying
	// to connect to the server; give up after 5s.
	int elapsedTime = 0;
	
	while (!(socketStream->isConnected()) )
	{
		try
		{
		    sleep(1); // a delay of 1s
		    
		    // try to connect to the server
		    socketStream->connectSocket(serverName, serverPortNo);
		    
		    // create an heartbeat thread
		    createHBThread();
		}
		catch (ConnectionError& )
		{
			// do nothing, if less than 5s ...
			if (++elapsedTime > MAX_ELAPSED_TIME)
			{
				// free up socketStream
				delete socketStream;

				// rethrow the exception
				throw;
			}
		}
	}
}

/*******************************************************************
* Function: Client::openSocketStream()
* Purpose : starts natlab and establish connection to natlab
* Initial : Nurudeen A. Lameed on May 5, 2009
********************************************************************
Revisions and bug fixes:
*/
void Client::openSocketStream(const char *svrName, const int svrPortNo)
{
	try
	{
		// attempt to start a natlab process
		start(svrName, svrPortNo);
		
		// try to establish a connection
		connect();
	}
	catch(ConnectionError& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		
		// for now, just exit the program
		exit(1);
	}
}

/*******************************************************************
* Function: Client::parseFile()
* Purpose : Sends parsefile command to the frontend
* Initial : Nurudeen A. Lameed on May 5, 2009
********************************************************************
Revisions and bug fixes:
*/
std::string Client::parseFile(const std::string& filePath)
{
	// build a command string
	std::string command = "<parsefile>" + XML::escapeString(filePath) + "</parsefile>";

	std::string reply = "";
	try
	{
		reply = sendCommand(command.c_str());
	}
	catch(std::exception& e)
	{
		reply = "<errorlist><error>" + std::string(e.what()) + "</error></errorlist>";
	}

	return reply;
}

/*******************************************************************
* Function: Client::parseText
* Purpose : Parses text
* Initial : Nurudeen A. Lameed on May 5, 2009
********************************************************************
Revisions and bug fixes:
*/
std::string Client::parseText(const std::string& txt)
{
	// build a command string
	std::string command = "<parsetext>" + XML::escapeString(txt) + "</parsetext>";

	std::string reply = "";
	try
	{
		reply = sendCommand(command.c_str());
	}
	catch(std::exception& e)
	{
		reply = "<errorlist><error>" + std::string(e.what()) + "</error></errorlist>";
	}

	return reply;
}

/*******************************************************************
* Function: Client::shutdown()
* Purpose : Shutdown the server (natlab)
* Initial : Nurudeen A. Lameed on May 5, 2009
********************************************************************
Revisions and bug fixes:
*/
std::string Client::shutdown()
{
	std::string reply = "";
	try
	{
		reply = sendCommand("<shutdown/>");
		closeSocketStream();
	}
	catch(std::exception& e)
	{
		reply = "<errorlist><error>" + XML::escapeString(std::string(e.what())) + "</error></errorlist>";
	}

	return reply;
}

/*******************************************************************
* Function: Client::runCommand()
* Purpose : send "command" to the frontend
* Initial : Nurudeen A. Lameed on May 5, 2009
********************************************************************
Revisions and bug fixes:
*/
std::string Client::sendCommand(const char* command)
{
	// acquire the mutual exclusion lock
	pthread_mutex_lock(&mutex);
	
	// send command, if connection is okay
	if (socketStream && socketStream->isConnected())
	{
		// send the command
		socketStream->sendAll(command, strlen(command) + 1);
		
		// release the mutual exclusion lock since only one thread is receiving
		pthread_mutex_unlock(&mutex);
		
		// return the output
		return socketStream->receiveUntilNull();
	}
	else
	{
		// must release the lock to progress
		pthread_mutex_unlock(&mutex);
		
		// an error has occured
		throw ConnectionError("Socket stream not available");
	}
}

/*******************************************************************
* Function: Client::waitHBThread()
* Purpose : wait for the heartbeat thread to terminate
* Initial : Nurudeen A. Lameed on June 1, 2009
********************************************************************
Revisions and bug fixes:
*/
inline void Client::waitHBThread()
{
	// wait for the heartbeat thread to terminate
	pthread_join(hb, 0);
}

/*******************************************************************
* Function: Client::createHBThread()
* Purpose : create a thread for sending heartbeat messages
* Initial : Nurudeen A. Lameed on June 1, 2009
********************************************************************
Revisions and bug fixes:
*/
inline int Client::createHBThread()
{	
	// create an heartbeat thread
	return pthread_create(&hb, 0, &heartbeat, 0);
}

/*******************************************************************
* Function: Client::heartbeat()
* Purpose : create a thread for sending heartbeat messages
* Initial : Nurudeen A. Lameed on June 1, 2009
********************************************************************
Revisions and bug fixes:
*/
void* Client::heartbeat(void* arg)
{
	// declare and initialize a heartbeat message
	const char* hb = "<heartbeat></heartbeat>";
			
	// send a heartbeat message to the server every MAX_INTERVAL secs
	while (true)
	{
		// acquire the mutual exclusion lock
		pthread_mutex_lock(&mutex);
     	
		// send command, if connection is okay
		if (socketStream && socketStream->isConnected())
		{
			try
			{
				// send the message 
				socketStream->sendAll(hb, strlen(hb) + 1);
     		
				// release the mutual exclusion lock since only one thread is receiving
				pthread_mutex_unlock(&mutex);
			}
			catch(std::exception& e)
			{
				// release the lock
				pthread_mutex_unlock(&mutex);
				
				std::cout << e.what() << std::endl;
												
				// stop, for now
				exit(1);
			}
		}
		else
		{
			// release the lock
			pthread_mutex_unlock(&mutex);
							
			std::cout << " Disconnected from the server " << std::endl;
															
			// stop, for now
			exit(1);
		}
		
		// wait for some seconds (interval)
		sleep(MAX_INTERVAL);
	}
}

/*******************************************************************
* Function: Client::closeSocketStream()
* Purpose : Releases stream resources used.
* Initial : Nurudeen A. Lameed on May 5, 2009
********************************************************************
Revisions and bug fixes:
*/
void Client::closeSocketStream()
{
	if (socketStream)
	{
		// close the socket
		socketStream->closeSocket();
		
		// free up the memory socketStream 
		delete socketStream;
		
		// clean up the socket
		ClientSocket::cleanUP();
	}
}
