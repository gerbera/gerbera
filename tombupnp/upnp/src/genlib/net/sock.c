///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000-2003 Intel Corporation 
// All rights reserved. 
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met: 
//
// * Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer. 
// * Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution. 
// * Neither name of Intel Corporation nor the names of its contributors 
// may be used to endorse or promote products derived from this software 
// without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////
/*TU*
    
    TombUPnP - a library for developing UPnP applications.
    
    Copyright (C) 2006-2010 Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License version 2.1 as published by the Free Software Foundation.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/************************************************************************
* Purpose: This file implements the sockets functionality 
************************************************************************/
#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "config.h"
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <poll.h>

#include "sock.h"
#include "upnp.h"
#include "upnpapi.h"

#ifndef WIN32
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <sys/time.h>
 #include <unistd.h>
#else
 #include <winsock2.h>
#endif
#include "unixutil.h"

#ifndef MSG_NOSIGNAL
 #define MSG_NOSIGNAL 0
#endif

/************************************************************************
*	Function :	sock_init
*
*	Parameters :
*		OUT SOCKINFO* info ;	Socket Information Object
*		IN int sockfd ;			Socket Descriptor
*
*	Description :	Assign the passed in socket descriptor to socket 
*		descriptor in the SOCKINFO structure.
*
*	Return : int;
*		UPNP_E_SUCCESS	
*		UPNP_E_OUTOF_MEMORY
*		UPNP_E_SOCKET_ERROR
*
*	Note :
************************************************************************/
int
sock_init( OUT SOCKINFO * info,
           IN int sockfd )
{
    assert( info );

    memset( info, 0, sizeof( SOCKINFO ) );

    info->socket = sockfd;

    return UPNP_E_SUCCESS;
}

/************************************************************************
*	Function :	sock_init_with_ip
*
*	Parameters :
*		OUT SOCKINFO* info ;				Socket Information Object
*		IN int sockfd ;						Socket Descriptor
*		IN struct in_addr foreign_ip_addr ;	Remote IP Address
*		IN unsigned short foreign_ip_port ;	Remote Port number
*
*	Description :	Calls the sock_init function and assigns the passed in
*		IP address and port to the IP address and port in the SOCKINFO
*		structure.
*
*	Return : int;
*		UPNP_E_SUCCESS	
*		UPNP_E_OUTOF_MEMORY
*		UPNP_E_SOCKET_ERROR
*
*	Note :
************************************************************************/
int
sock_init_with_ip( OUT SOCKINFO * info,
                   IN int sockfd,
                   IN struct in_addr foreign_ip_addr,
                   IN unsigned short foreign_ip_port )
{
    int ret;

    ret = sock_init( info, sockfd );
    if( ret != UPNP_E_SUCCESS ) {
        return ret;
    }

    info->foreign_ip_addr = foreign_ip_addr;
    info->foreign_ip_port = foreign_ip_port;

    return UPNP_E_SUCCESS;
}

/************************************************************************
*	Function :	sock_destroy
*
*	Parameters :
*		INOUT SOCKINFO* info ;	Socket Information Object
*		int ShutdownMethod ;	How to shutdown the socket. Used by  
*								sockets's shutdown() 
*
*	Description :	Shutsdown the socket using the ShutdownMethod to 
*		indicate whether sends and receives on the socket will be 
*		dis-allowed. After shutting down the socket, closesocket is called
*		to release system resources used by the socket calls.
*
*	Return : int;
*		UPNP_E_SOCKET_ERROR on failure
*		UPNP_E_SUCCESS on success
*
*	Note :
************************************************************************/
int
sock_destroy( INOUT SOCKINFO * info,
              int ShutdownMethod )
{
    shutdown( info->socket, ShutdownMethod );
    if( UpnpCloseSocket( info->socket ) == -1 ) {
        return UPNP_E_SOCKET_ERROR;
    }

    return UPNP_E_SUCCESS;
}

/************************************************************************
*	Function :	sock_read_write
*
*	Parameters :
*		IN SOCKINFO *info ;	Socket Information Object
*		OUT char* buffer ;	Buffer to get data to or send data from 
*		IN size_t bufsize ;	Size of the buffer
*	    IN int *timeoutSecs ;	timeout value
*		IN xboolean bRead ;	Boolean value specifying read or write option
*
*	Description :	Receives or sends data. Also returns the time taken
*		to receive or send data.
*
*	Return :int ;
*		numBytes - On Success, no of bytes received or sent		
*		UPNP_E_TIMEDOUT - Timeout
*		UPNP_E_SOCKET_ERROR - Error on socket calls
*
*	Note :
************************************************************************/
#if 0
static int
sock_read_write( IN SOCKINFO * info,
                 OUT char *buffer,
                 IN size_t bufsize,
                 IN int *timeoutSecs,
                 IN xboolean bRead )
{
    int retCode;
    fd_set readSet;
    fd_set writeSet;
    struct timeval timeout;
    int numBytes;
    time_t start_time = time( NULL );
    int sockfd = info->socket;
    long bytes_sent = 0,
      byte_left = 0,
      num_written;

    if( *timeoutSecs < 0 ) {
        return UPNP_E_TIMEDOUT;
    }

    FD_ZERO( &readSet );
    FD_ZERO( &writeSet );
    if( bRead ) {
        FD_SET( ( unsigned )sockfd, &readSet );
    } else {
        FD_SET( ( unsigned )sockfd, &writeSet );
    }

    timeout.tv_sec = *timeoutSecs;
    timeout.tv_usec = 0;

    while( TRUE ) {
        if( *timeoutSecs == 0 ) {
            retCode =
                select( sockfd + 1, &readSet, &writeSet, NULL, NULL );
        } else {
            retCode =
                select( sockfd + 1, &readSet, &writeSet, NULL, &timeout );
        }

        if( retCode == 0 ) {
            return UPNP_E_TIMEDOUT;
        }
        if( retCode == -1 ) {
            if( errno == EINTR )
                continue;
            return UPNP_E_SOCKET_ERROR; // error
        } else {
            break;              // read or write
        }
    }

    if( bRead ) {
        // read data
        numBytes = recv( sockfd, buffer, bufsize,MSG_NOSIGNAL);
    } else {
        byte_left = bufsize;
        bytes_sent = 0;
        while( byte_left > 0 ) {
            // write data
            num_written =
                send( sockfd, buffer + bytes_sent, byte_left,
                      MSG_DONTROUTE|MSG_NOSIGNAL);
            if( num_written == -1 ) {
                return num_written;
            }

            byte_left = byte_left - num_written;
            bytes_sent += num_written;
        }

        numBytes = bytes_sent;
    }

    if( numBytes < 0 ) {
        return UPNP_E_SOCKET_ERROR;
    }
    // subtract time used for reading/writing
    if( *timeoutSecs != 0 ) {
        *timeoutSecs -= time( NULL ) - start_time;
    }

    return numBytes;
}
#endif

/************************************************************************
*	Function :	sock_read
*
*	Parameters :
*		IN SOCKINFO *info ;	Socket Information Object
*		OUT char* buffer ;	Buffer to get data to  
*		IN size_t bufsize ;	Size of the buffer
*	    IN int *timeoutSecs ;	timeout value
*
*	Description :	Calls sock_read_write() for reading data on the 
*		socket
*
*	Return : int;
*		Values returned by sock_read_write() 
*
*	Note :
************************************************************************/
int
sock_read( IN SOCKINFO * info,
           OUT char *buffer,
           IN size_t bufsize,
           INOUT int *timeoutSecs )
{
//    return sock_read_write( info, buffer, bufsize, timeoutSecs, TRUE );
    int sockfd = info->socket; 
    int retCode;
    fd_set readSet;
    struct timeval timeout;
    ssize_t bytes_received = 0;
    int num_bytes = 0;
    char* p_buffer = buffer;
    int no_timeout = 0; // detect end of data

    if (*timeoutSecs < 0)
    {
        return UPNP_E_TIMEDOUT;
    }

    FD_ZERO(&readSet);
    FD_SET(sockfd, &readSet);

    timeout.tv_sec = *timeoutSecs;
    timeout.tv_usec = 0;
       
    while (TRUE)
    {
        if (*timeoutSecs == 0)
        {
            retCode = select(sockfd + 1, &readSet, NULL, NULL, NULL);
        }
        else
        {
            retCode = select(sockfd + 1, &readSet, NULL, NULL, &timeout);
        }

        if (retCode == -1)
        {
            if(errno == EINTR)
                continue;

            return UPNP_E_SOCKET_ERROR;
        }
        
        if (retCode == 0)
        {
            if (no_timeout)
                return num_bytes;
            else
                return UPNP_E_TIMEDOUT;
        }

        if (FD_ISSET(sockfd, &readSet))
        {
            // we use this to detect "end of data" so we do not block
            // when it may not be needed
            // only the first select will wait for a timeout (in case a
            // timeout was specified)
            timeout.tv_sec = 0; 
            timeout.tv_usec = 0;
            if (*timeoutSecs)
                no_timeout = 1;

            bytes_received = recv(sockfd, p_buffer, bufsize, MSG_NOSIGNAL);

            // nothing more to read
            if (bytes_received == 0)
                break;

            // error
            if (bytes_received < 0)
                return UPNP_E_SOCKET_ERROR;

            num_bytes = num_bytes + bytes_received;
            bufsize = bufsize - bytes_received;

            // no space left in buffer
            if (bufsize <= 0)
                break;

            p_buffer = buffer + num_bytes;
        }
    }

    if (num_bytes < 0)  
    {
        return UPNP_E_SOCKET_ERROR;
    }

    return num_bytes;
}

// checks if writing to the socket is possible
int
sock_check_w( IN SOCKINFO * info )
{
    fd_set readSet;
    fd_set exceptSet;
    struct timeval timeout;

    int retCode;
    int sockfd = info->socket;

    FD_ZERO(&readSet);
    FD_ZERO(&exceptSet);
    FD_SET(sockfd, &readSet);
    FD_SET(sockfd, &exceptSet);

    timeout.tv_sec = 1; // 1 second select, we do not care if it times out
                        // we only look for an error
    timeout.tv_usec = 0;

    retCode = select(sockfd + 1, &readSet, NULL, &exceptSet, &timeout);
    
    if (FD_ISSET(sockfd, &readSet) || FD_ISSET(sockfd, &exceptSet))
    {
        return 0;
    }

    if (retCode >= 0) // timeout or fd ready for writing
        return 1;

    if (retCode == -1)
    {
        if(errno == EINTR)
            return 1;
    }

    return 0;
}

/************************************************************************
*	Function :	sock_write
*
*	Parameters :
*		IN SOCKINFO *info ;	Socket Information Object
*		IN char* buffer ;	Buffer to send data from 
*		IN size_t bufsize ;	Size of the buffer
*	    IN int *timeoutSecs ;	timeout value
*
*	Description :	Calls sock_read_write() for writing data on the 
*		socket
*
*	Return : int;
*		sock_read_write()
*
*	Note :
************************************************************************/
int
sock_write( IN SOCKINFO * info,
            IN char *buffer,
            IN size_t bufsize,
            INOUT int *timeoutSecs )
{
//    return sock_read_write( info, buffer, bufsize, timeoutSecs, FALSE );
    int sockfd = info->socket;
    int retCode;
    fd_set writeSet;
    struct timeval timeout;
    ssize_t bytes_sent = 0;
    int num_bytes = 0;
    char* p_buffer = buffer;
    int retry = 0;

    if (*timeoutSecs < 0)
    {
        return UPNP_E_TIMEDOUT;
    }

    while (TRUE)
    {
        FD_ZERO(&writeSet);
        FD_SET(sockfd, &writeSet);

        timeout.tv_sec = *timeoutSecs;
        timeout.tv_usec = 0;

        if (*timeoutSecs == 0)
        {
            timeout.tv_sec = WEB_SERVER_BLOCK_TIMEOUT;
        }
        
        retCode = select(sockfd + 1, NULL, &writeSet, NULL, &timeout);

        if (gUpnpSdkShutdown)
            return UPNP_E_TIMEDOUT;

        if (retCode == 0)
        {
            if (*timeoutSecs != 0)
                return UPNP_E_TIMEDOUT;

            if (gMaxHTTPTimeoutRetries > 0)
            {
                retry++;
                if (retry >= gMaxHTTPTimeoutRetries)
                    return UPNP_E_TIMEDOUT;
            }

            continue;
        }

        if (retCode == -1)
        {
            if(errno == EINTR)
                continue;

            return UPNP_E_SOCKET_ERROR;
        }

        if (FD_ISSET(sockfd, &writeSet))
        {
            bytes_sent = send(sockfd, p_buffer, bufsize, MSG_DONTROUTE|MSG_NOSIGNAL);
            if (bytes_sent < 0)
            {
                //return -1;
                return UPNP_E_SOCKET_ERROR;
            }

            num_bytes = num_bytes + bytes_sent;
            bufsize = bufsize - bytes_sent;

            // all sent out 
            if (bufsize <= 0)
                break;

            p_buffer = buffer + num_bytes;
        }
    }

    if (num_bytes < 0) 
    {
        return UPNP_E_SOCKET_ERROR;
    }

    return num_bytes;
}
