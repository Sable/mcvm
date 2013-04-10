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

/* preprocessor directives for cross-platform portability */
#ifndef SOCK_H_
#define SOCK_H_
#ifdef WIN32		/* Windows specifics */
   #include <winsock.h>
   #define Socket SOCKET
   #define close closesocket
   #define socklen_t int
   #pragma comment(lib, "wsock32.lib")
   #define _WSA_STARTUP(){\
             WSADATA info; \
             WSAStartup(MAKEWORD(2,0), &info);\
           }
   #define _WSA_CLEANUP() WSACleanup();
#else			/* Others - Unix and its variants */
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <arpa/inet.h>
   #include <netdb.h>
   #define Socket int
   #define _WSA_STARTUP() {}
   #define _WSA_CLEANUP() {}
#endif
#include <cstring>
#include <stdexcept>
#include <cstdio>
extern "C" {
   #include <errno.h>
}
extern int errno;

#endif /* SOCK_H_ */

