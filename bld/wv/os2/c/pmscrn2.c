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
* Description:  OS/2 PM specific debugger code dealing with multithreading.
*               Scary stuff.
*
****************************************************************************/


#define INCL_SUB
#define INCL_PM
#define INCL_BASE
#define INCL_WINSYS
#define INCL_WINHOOKS
#define INCL_WINMESSAGEMGR
#include <os2.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "dbgmem.h"
#include "trptypes.h"
#include "trapaccs.h"
#include "dbgscrn.h"
#include "wndsys.h"
#include "trpld.h"
#include "trpsys.h"
#include "dbginit.h"
#include "guigsysh.h"


extern BOOL APIENTRY WinThreadAssocQueue(HAB, HMQ);

extern HMQ              GUIPMmq;

unsigned                NumLines;
unsigned                NumColumns;
bool                    TrapForceHardMode = false;
bool                    ToldWinHandle = false;
//TODO: see if these two event sems could be replaced by single mutex
HEV                     PumpMessageSem = NULLHANDLE;
HEV                     PumpMessageDoneSem = NULLHANDLE;
#define STACK_SIZE      32768


void WndInitWndMain( wnd_create_struct *info )
{
    WndInitCreateStruct( info );
    info->style |= GUI_INIT_INVISIBLE;
}

void TellWinHandle( void )
{
    if( !ToldWinHandle && TRAP_EXTFUNC( TellHandles )( GUIGetHAB(), GUIGetSysHandle( WndGui( WndMain ) ) ) ) {
        ToldWinHandle = true;
    }
}

void ToggleHardMode( void )
{
}

void Ring_Bell( void )
{
    DosBeep( 1000, 250 );
}


/*
 * ConfigScreen -- figure out screen configuration we're going to use.
 */

unsigned ConfigScreen( void )
{
    return( 0 );
}


size_t GetSystemDir( char *buff, size_t buff_len )
/************************************************/
{
    // inst
    PRFPROFILE                  prof;
    size_t                      i;

    prof.cchUserName = 0L;
    prof.cchSysName = 0L;

    i = 0;
    if (PrfQueryProfile(GUIGetHAB(), &prof)) {
        if (prof.cchSysName > 0) {
            _AllocA(prof.pszSysName, prof.cchSysName);
            _AllocA(prof.pszUserName, prof.cchUserName);
            PrfQueryProfile(GUIGetHAB(), &prof);
        }

        if( buff_len > prof.cchSysName )
            buff_len = prof.cchSysName;
        memcpy( buff, prof.pszSysName, buff_len );
        if( buff_len > 0 )
            i = --buff_len;     // reserve space for terminating null character

        while( i > 0 ) {
            if( buff[--i] == '\\' ) {
                break;
            }
        }
    }
    buff[i] = NULLCHAR;
    return( strlen( buff ) );
} /* _wpi_getinidirectory */

/*
 * InitScreen
 */

static void APIENTRY PumpMessageQueue( ULONG parm )
{
    char        class_name[80];
    QMSG        qmsg;
    ERRORID     err;
    ULONG       ulCount;

    parm=parm;

    for( ;; ) {
        DosWaitEventSem( PumpMessageSem, SEM_INDEFINITE_WAIT );
        DosResetEventSem( PumpMessageSem, &ulCount );
        WinThreadAssocQueue( GUIGetHAB(), GUIPMmq );
        while( WinGetMsg( GUIGetHAB(), &qmsg, 0L, 0, 0 ) ) {
            WinQueryClassName( qmsg.hwnd, sizeof( class_name ), class_name );
            if (strcmp( class_name, "GUIClass" ) == 0 ||
                strcmp( class_name, "WTool" ) == 0) {
                WinDefWindowProc( qmsg.hwnd, qmsg.msg, qmsg.mp1, qmsg.mp2 );
            } else {
                WinDispatchMsg( GUIGetHAB(), &qmsg );
            }
        }
        WinThreadAssocQueue( GUIGetHAB(), NULLHANDLE );
        err = WinGetLastError( GUIGetHAB() );
        DosPostEventSem( PumpMessageDoneSem );
    }
}

void InitScreen( void )
{
    TID                 tid;

    RestoreMainScreen( "WDPM" );
    DosCreateEventSem( NULL, &PumpMessageDoneSem, 0, false );
    DosCreateEventSem( NULL, &PumpMessageSem, 0, false );
    DosCreateThread( &tid, PumpMessageQueue, 0, 0, STACK_SIZE );
    DosSetPriority( PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, tid );
}


/*
 * UsrScrnMode -- setup the user screen mode
 */

bool UsrScrnMode( void )
{
    return( false );
}


void DbgScrnMode( void )
{
}


/*
 * DebugScreen -- swap/page to debugger screen
 */

static HWND FocusWnd, ActiveWnd;

bool DebugScreen( void )
{
    if( !WndMain )
        return( false );
    if( FocusWnd && WinIsWindow( GUIGetHAB(), FocusWnd ) &&
        FocusWnd != WinQueryFocus( HWND_DESKTOP ) ) {
        WinSetFocus(HWND_DESKTOP, FocusWnd);
    }
    if( ActiveWnd && WinIsWindow( GUIGetHAB(), ActiveWnd ) &&
        ActiveWnd != WinQueryActiveWindow( HWND_DESKTOP ) ) {
        WinSetActiveWindow( HWND_DESKTOP, ActiveWnd );
    }
    return( false );
}


bool DebugScreenRecover( void )
{
    return( true );
}


/*
 * UserScreen -- swap/page to user screen
 */

bool UserScreen( void )
{
    if( !WndMain )
        return( false );
    FocusWnd = WinQueryFocus( HWND_DESKTOP );
    ActiveWnd = WinQueryActiveWindow( HWND_DESKTOP );
    return( false );
}

void SaveMainWindowPos( void )
{
    SaveMainScreen( "WDPM" );
}

void FiniScreen( void )
{
    DosCloseEventSem( PumpMessageSem );
    DosCloseEventSem( PumpMessageDoneSem );
}


/*****************************************************************************\
 *                                                                           *
 *            Replacement routines for User Interface library                *
 *                                                                           *
\*****************************************************************************/

void PopErrBox( const char *buff )
{
    WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, buff,
                  LIT_ENG(Debugger_Startup_Error), 1001,
                  MB_MOVEABLE | MB_CUACRITICAL | MB_CANCEL );
}

unsigned OnAnotherThreadAccess( trap_elen in_num, in_mx_entry_p in_mx, trap_elen out_num, mx_entry_p out_mx )
{
    unsigned    result;
    ULONG       ulCount;

    if( !ToldWinHandle || IsTrapFilePumpingMessageQueue() ) {
        return( TrapAccess( in_num, in_mx, out_num, out_mx ) );
    } else {
        DosPostEventSem( PumpMessageSem );
        result = TrapAccess( in_num, in_mx, out_num, out_mx );
        WinPostMsg( GUIGetSysHandle( WndGui( WndMain ) ), WM_QUIT, 0, 0 );
        DosWaitEventSem( PumpMessageDoneSem, SEM_INDEFINITE_WAIT );
        DosResetEventSem( PumpMessageDoneSem, &ulCount );
        return result;
    }
}

unsigned OnAnotherThreadSimpleAccess( trap_elen in_len, in_data_p in_data, trap_elen out_len, out_data_p out_data )
{
    unsigned    result;
    ULONG       ulCount;

    if( !ToldWinHandle || IsTrapFilePumpingMessageQueue() ) {
        return( TrapSimpleAccess( in_len, in_data, out_len, out_data ) );
    } else {
        DosPostEventSem( PumpMessageSem );
        result = TrapSimpleAccess( in_len, in_data, out_len, out_data );
        WinPostMsg( GUIGetSysHandle( WndGui( WndMain ) ), WM_QUIT, 0, 0 );
        DosWaitEventSem( PumpMessageDoneSem, SEM_INDEFINITE_WAIT );
        DosResetEventSem( PumpMessageDoneSem, &ulCount );
        return result;
    }
}

void SetNumLines( int num )
{
    if( num < 10 )
        num = 10;
    if( num > 99 )
        num = 99;
    NumLines = num;
}

void SetNumColumns( int num )
{
    if( num < 25 )
        num = 25;
    if( num > 255 )
        num = 255;
    NumColumns = num;
}

bool ScreenOption( const char *start, unsigned len, int pass )
{
    start=start;len=len;pass=pass;
    return( false );
}

void ScreenOptInit( void )
{
}
