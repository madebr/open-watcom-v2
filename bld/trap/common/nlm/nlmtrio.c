/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2022 The Open Watcom Contributors. All Rights Reserved.
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "miniproc.h"
#include "debugme.h"
#include <string.h>
#undef POP_UP_SCREEN
#define ConsolePrintf _
//#include <conio.h>
#undef ConsolePrintf
#include "nw3to5.h"
#include "servio.h"
#include "nlmlibc.h"
#if !defined( __NETWARE_LIBC__ )
    #include <ecb.h>
#endif


extern struct ScreenStruct                     *screenID;
extern struct LoadDefinitionStruct             *MyNLMHandle;

void Output( const char *str )
{
    ActivateScreen( screenID );
    OutputToScreen( screenID, "%s", str );
    SetInputToOutputCursorPosition( screenID );
}


void SayGNiteGracey( int return_code )
{
    return_code = return_code;
    KillMe( MyNLMHandle );
#if defined( __NETWARE_LIBC__ )
    NXVmExit(return_code);
    // never return
#else
    CDestroyProcess( CGetMyProcessID() );
    // never return
#endif
}

void StartupErr( const char *err )
{
    OutputToScreen( systemConsoleScreen, "%s\r\n", err );
    SayGNiteGracey( 1 );
    // never return
}

int KeyPress( void )
{
   return( CheckKeyStatus( screenID ) ); /* RELINQUISH ? */
}

int KeyGet( void )
{
   BYTE value, scanCode, type;

   SetInputToOutputCursorPosition( screenID );
   GetKey( screenID, &type, &value, NULL, &scanCode, 0); /* RELINQUISH ? */
   return( value );
}

int WantUsage( const char *ptr )
{
    return( *ptr == '?' );
}
