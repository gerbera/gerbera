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

#include "upnp_config.h"
#include <stdlib.h>
#include <stdio.h>
#include "ithread.h"
#include "upnp.h"
#include <stdarg.h>

#ifdef DEBUG
#include <string.h>
#endif

//Mutex to synchronize all the log file opeartions in the debug mode
ithread_mutex_t GlobalDebugMutex;

//File handle for the error log file
FILE *ErrFileHnd = NULL;

//File handle for the information log file
FILE *InfoFileHnd = NULL;

//Name of the error file
const char *errFileName = "IUpnpErrFile.txt";

//Name of the info file
const char *infoFileName = "IUpnpInfoFile.txt";

/************************************************************************
* Function : UpnpDisplayBanner											
*																	
* Parameters:														
*	IN FILE *fd: file descriptor where the banner will be written
*	IN char **lines: The buffer that will be written
*	IN int size: Size of the buffer
*	IN int starLength: This parameter provides the width of the banner
*																	
* Description:														
*	This functions takes the buffer and writes the buffer in the file as 
*	per the requested banner											
* Returns: void
***************************************************************************/
void UpnpDisplayBanner( IN FILE * fd,
                        IN char **lines,
                        IN int size,
                        IN int starLength );

/************************************************************************
* Function : SetLogFileNames											
*																	
* Parameters:														
*	IN const char* ErrFileName: name of the error file
*	IN const char *InfoFileName: name of the information file
*	IN int size: Size of the buffer
*	IN int starLength: This parameter provides the width of the banner
*																	
* Description:														
*	This functions takes the buffer and writes the buffer in the file as 
*	per the requested banner											
* Returns: void
***************************************************************************/
void
SetLogFileNames( IN const char *ErrFileName,
                 IN const char *InfoFileName )
{
    if( ErrFileName ) {
        errFileName = ErrFileName;
    }
    if( InfoFileName ) {
        infoFileName = InfoFileName;
    }
}

/************************************************************************
* Function : InitLog											
*																	
* Parameters:	void													
*																	
* Description:														
*	This functions initializes the log files
* Returns: int
*	-1 : If fails
*	UPNP_E_SUCCESS : if success
***************************************************************************/
int
InitLog(  )
{
    ithread_mutex_init( &GlobalDebugMutex, NULL );

    if( DEBUG_TARGET == 1 ) {
        if( ( ErrFileHnd = fopen( errFileName, "a" ) ) == NULL )
            return -1;
        if( ( InfoFileHnd = fopen( infoFileName, "a" ) ) == NULL )
            return -1;
    }
    return UPNP_E_SUCCESS;
}

/************************************************************************
* Function : CloseLog											
*																	
* Parameters:	void													
*																	
* Description:														
*	This functions closes the log files
* Returns: void
***************************************************************************/
void
CloseLog(  )
{
    if( DEBUG_TARGET == 1 ) {
        fflush( ErrFileHnd );
        fflush( InfoFileHnd );
        fclose( ErrFileHnd );
        fclose( InfoFileHnd );
    }
    ithread_mutex_destroy( &GlobalDebugMutex );

}

/**************************************************************************
* Function : UpnpPrintf											
*																	
* Parameters:														
*	IN Dbg_Level DLevel: The level of the debug logging. It will decide 
*		whether debug statement will go to standard output, or any of the 
*		log files.
*	IN Dbg_Module Module: debug will go in the name of this module
*	IN char *DbgFileName: Name of the file from where debug statement is
*							coming
*	IN int DbgLineNo : Line number of the file from where debug statement is
*							coming
*	IN char * FmtStr, ...: Variable number of arguments that will go in the 
*			debug statement
*																	
* Description:														
*	This functions prints the debug statement either on the startdard output
*	or log file along with the information from where this debug statement 
*	is coming
* Returns: void
***************************************************************************/
DBGONLY( void UpnpPrintf( IN Dbg_Level DLevel,
                          IN Dbg_Module Module,
                          IN char *DbgFileName,
                          IN int DbgLineNo,
                          IN char *FmtStr,
                          ... ) {

         va_list ArgList;
         va_start( ArgList, FmtStr );
         if( DEBUG_LEVEL < DLevel ) return; if( DEBUG_ALL == 0 ) {
         switch ( Module ) {
case SSDP:
         if( DEBUG_SSDP == 1 ) break;
         else
return; case SOAP:
         if( DEBUG_SOAP == 1 ) break;
         else
return; case GENA:
         if( DEBUG_GENA == 1 ) break;
         else
return; case TPOOL:
         if( DEBUG_TPOOL == 1 ) break;
         else
return; case MSERV:
         if( DEBUG_MSERV == 1 ) break;
         else
return; case DOM:
         if( DEBUG_DOM == 1 ) break;
         else
return; case HTTP:
         if( DEBUG_HTTP == 1 ) break;
         else
return; case API:
         if( DEBUG_API == 1 ) break;
         else
return; default:
         return;}
         }

         ithread_mutex_lock( &GlobalDebugMutex ); if( DEBUG_TARGET == 0 ) {
         if( DbgFileName ) {
         UpnpDisplayFileAndLine( stdout, DbgFileName, DbgLineNo );}
         vfprintf( stdout, FmtStr, ArgList ); fflush( stdout );}
         else
         {
         if( DLevel == 0 ) {
         if( DbgFileName ) {
         UpnpDisplayFileAndLine( ErrFileHnd, DbgFileName, DbgLineNo );}
         vfprintf( ErrFileHnd, FmtStr, ArgList ); fflush( ErrFileHnd );}
         else
         {
         if( DbgFileName ) {
         UpnpDisplayFileAndLine( InfoFileHnd, DbgFileName, DbgLineNo );}
         vfprintf( InfoFileHnd, FmtStr, ArgList ); fflush( InfoFileHnd );}
         }
         va_end( ArgList ); ithread_mutex_unlock( &GlobalDebugMutex );}

 )

/**************************************************************************
* Function : GetDebugFile											
*																	
* Parameters:														
*	IN Dbg_Level DLevel: The level of the debug logging. It will decide 
*		whether debug statement will go to standard output, or any of the 
*		log files.
*	IN Dbg_Module Module: debug will go in the name of this module
*																	
* Description:
*	This function checks if the module is turned on for debug 
*	and returns the file descriptor corresponding to the debug level
* Returns: FILE *
*	NULL : if the module is turn off for debug 
*	else returns the right file descriptor
***************************************************************************/
    DBGONLY( FILE * GetDebugFile( Dbg_Level DLevel, Dbg_Module Module ) {
             if( DEBUG_LEVEL < DLevel ) return NULL; if( DEBUG_ALL == 0 ) {
             switch ( Module ) {
case SSDP:
             if( DEBUG_SSDP == 1 ) break;
             else
return NULL; case SOAP:
             if( DEBUG_SOAP == 1 ) break;
             else
return NULL; case GENA:
             if( DEBUG_GENA == 1 ) break;
             else
return NULL; case TPOOL:
             if( DEBUG_TPOOL == 1 ) break;
             else
return NULL; case MSERV:
             if( DEBUG_MSERV == 1 ) break;
             else
return NULL; case DOM:
             if( DEBUG_DOM == 1 ) break;
             else
return NULL; case API:
             if( DEBUG_API == 1 ) break;
             else
return NULL; default:
             return NULL;}
             }

             if( DEBUG_TARGET == 0 ) {
             return stdout;}
             else
             {
             if( DLevel == 0 ) {
             return ErrFileHnd;}
             else
             {
             return InfoFileHnd;}
             }
             }
 )

/**************************************************************************
* Function : UpnpDisplayFileAndLine											
*																	
* Parameters:														
*	IN FILE *fd: File descriptor where line number and file name will be 
*			written 
*	IN char *DbgFileName: Name of the file  
*	IN int DbgLineNo : Line number of the file
*																	
* Description:
*	This function writes the file name and file number from where
*		debug statement is coming to the log file
* Returns: void
***************************************************************************/
    DBGONLY( void UpnpDisplayFileAndLine( IN FILE * fd,
                                          IN char *DbgFileName,
                                          IN int DbgLineNo ) {
             int starlength = 66;
             char *lines[2];
             char FileAndLine[500]; lines[0] = "DEBUG"; if( DbgFileName ) {
             sprintf( FileAndLine, "FILE: %s, LINE: %d", DbgFileName,
                      DbgLineNo ); lines[1] = FileAndLine;}

             UpnpDisplayBanner( fd, lines, 2, starlength ); fflush( fd );}
 )

/************************************************************************
* Function : UpnpDisplayBanner											
*																	
* Parameters:														
*	IN FILE *fd: file descriptor where the banner will be written
*	IN char **lines: The buffer that will be written
*	IN int size: Size of the buffer
*	IN int starLength: This parameter provides the width of the banner
*																	
* Description:														
*	This functions takes the buffer and writes the buffer in the file as 
*	per the requested banner											
* Returns: void
***************************************************************************/
    DBGONLY( void UpnpDisplayBanner( IN FILE * fd,
                                     IN char **lines,
                                     IN int size,
                                     IN int starLength ) {
             char *stars = ( char * )malloc( starLength + 1 );
             char *line = NULL;
             int leftMarginLength = starLength / 2 + 1;
             int rightMarginLength = starLength / 2 + 1;
             char *leftMargin = ( char * )malloc( leftMarginLength );
             char *rightMargin = ( char * )malloc( rightMarginLength );
             int i = 0;
             int LineSize = 0;
             char *currentLine = ( char * )malloc( starLength + 1 );
             memset( stars, '*', starLength );
             stars[starLength] = 0;
             memset( leftMargin, 0, leftMarginLength );
             memset( rightMargin, 0, rightMarginLength );
             fprintf( fd, "\n%s\n", stars ); for( i = 0; i < size; i++ ) {
             LineSize = strlen( lines[i] );
             line = lines[i]; while( LineSize > ( starLength - 2 ) ) {
             memcpy( currentLine, line, ( starLength - 2 ) );
             currentLine[( starLength - 2 )] = 0;
             fprintf( fd, "*%s*\n", currentLine );
             LineSize -= ( starLength - 2 ); line += ( starLength - 2 );}

             if( LineSize % 2 == 0 ) {
             leftMarginLength = rightMarginLength =
             ( ( starLength - 2 ) - LineSize ) / 2;}
             else
             {
             leftMarginLength = ( ( starLength - 2 ) - LineSize ) / 2;
             rightMarginLength =
             ( ( starLength - 2 ) - LineSize ) / 2 + 1;}

             memset( leftMargin, ' ', leftMarginLength );
             memset( rightMargin, ' ', rightMarginLength );
             leftMargin[leftMarginLength] = 0;
             rightMargin[rightMarginLength] = 0;
             fprintf( fd, "*%s%s%s*\n", leftMargin, line, rightMargin );}

             fprintf( fd, "%s\n\n", stars );
             free( leftMargin );
             free( rightMargin ); free( stars ); free( currentLine );}
 )
