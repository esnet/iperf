// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1993-1997  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   service.c
//
//  PURPOSE:  Implements functions required by all services
//            windows.
//
//  FUNCTIONS:
//    main(int argc, char **argv);
//    service_ctrl(DWORD dwCtrlCode);
//    service_main(DWORD dwArgc, LPTSTR *lpszArgv);
//    CmdInstallService();
//    CmdRemoveService();
//    ControlHandler ( DWORD dwCtrlType );
//    GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );
//
//  COMMENTS:
//
//  AUTHOR: Craig Link - Microsoft Developer Support
//

/*
 * modified Mar.07, 2002 by Feng Qin <fqin@ncsa.uiuc.edu>
 *          Mar.15, 2002
 *
 * removed some functions we don't use at all
 * add code to start the service immediately after service is installed
 * 
 * $Id: service.c,v 1.1.1.1 2004/05/18 01:50:44 kgibbs Exp $
 */

#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>

#include "service.h"



// internal variables
SERVICE_STATUS          ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   dwErr = 0;
TCHAR                   szErr[256];

//
//  FUNCTION: service_main
//
//  PURPOSE: To perform actual initialization of the service
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    This routine performs the service initialization and then calls
//    the user defined ServiceStart() routine to perform majority
//    of the work.
//
void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv) {
    // register our service control handler:
    //
    sshStatusHandle = RegisterServiceCtrlHandler( TEXT(SZSERVICENAME), service_ctrl);

    if ( !sshStatusHandle )
        goto clean;

    // SERVICE_STATUS members that don't change in example
    //
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;

    // report the status to the service control manager.
    //
    if ( !ReportStatusToSCMgr(
                             SERVICE_START_PENDING, // service state
                             NO_ERROR,              // exit code
                             3000) )                 // wait hint
        goto clean;


    ServiceStart( dwArgc, lpszArgv );

    clean:

    // try to report the stopped status to the service control manager.
    //
    if ( sshStatusHandle )
        (VOID)ReportStatusToSCMgr(
                                 SERVICE_STOPPED,
                                 dwErr,
                                 0);

    return;
}



//
//  FUNCTION: service_ctrl
//
//  PURPOSE: This function is called by the SCM whenever
//           ControlService() is called on this service.
//
//  PARAMETERS:
//    dwCtrlCode - type of control requested
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID WINAPI service_ctrl(DWORD dwCtrlCode) {
    // Handle the requested control code.
    //
    switch ( dwCtrlCode ) {
        // Stop the service.
        //
        // SERVICE_STOP_PENDING should be reported before
        // setting the Stop Event - hServerStopEvent - in
        // ServiceStop().  This avoids a race condition
        // which may result in a 1053 - The Service did not respond...
        // error.
        case SERVICE_CONTROL_STOP:
            ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0);
            ServiceStop();

            return;

            // Update the service status.
            //
        case SERVICE_CONTROL_INTERROGATE:
            break;

            // invalid control code
            //
        default:
            break;

    }
    ReportStatusToSCMgr(SERVICE_STOPPED, NO_ERROR, 0);
//    ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0);
}



//
//  FUNCTION: ReportStatusToSCMgr()
//
//  PURPOSE: Sets the current status of the service and
//           reports it to the Service Control Manager
//
//  PARAMETERS:
//    dwCurrentState - the state of the service
//    dwWin32ExitCode - error code to report
//    dwWaitHint - worst case estimate to next checkpoint
//
//  RETURN VALUE:
//    TRUE  - success
//    FALSE - failure
//
//  COMMENTS:
//
BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                         DWORD dwWin32ExitCode,
                         DWORD dwWaitHint) {
    static DWORD dwCheckPoint = 1;
    BOOL fResult = TRUE;


    if ( dwCurrentState == SERVICE_START_PENDING )
        ssStatus.dwControlsAccepted = 0;
    else
        ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    ssStatus.dwCurrentState = dwCurrentState;
    ssStatus.dwWin32ExitCode = dwWin32ExitCode;
    ssStatus.dwWaitHint = dwWaitHint;

    if ( ( dwCurrentState == SERVICE_RUNNING ) ||
         ( dwCurrentState == SERVICE_STOPPED ) )
        ssStatus.dwCheckPoint = 0;
    else
        ssStatus.dwCheckPoint = dwCheckPoint++;


    // Report the status of the service to the service control manager.
    //
    if ( !(fResult = SetServiceStatus( sshStatusHandle, &ssStatus)) ) {
        AddToMessageLog(TEXT("SetServiceStatus"));
    }
    return fResult;
}



//
//  FUNCTION: AddToMessageLog(LPTSTR lpszMsg)
//
//  PURPOSE: Allows any thread to log an error message
//
//  PARAMETERS:
//    lpszMsg - text for message
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID AddToMessageLog(LPTSTR lpszMsg) {
    TCHAR   szMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[2];


    dwErr = GetLastError();

    // Use event logging to log the error.
    //
    hEventSource = RegisterEventSource(NULL, TEXT(SZSERVICENAME));

    printf(lpszMsg);

    _stprintf(szMsg, TEXT("%s error: %d"), TEXT(SZSERVICENAME), dwErr);
    lpszStrings[0] = szMsg;
    lpszStrings[1] = lpszMsg;

    if ( hEventSource != NULL ) {
        ReportEvent(hEventSource, // handle of event source
                    EVENTLOG_ERROR_TYPE,  // event type
                    0,                    // event category
                    0,                    // event ID
                    NULL,                 // current user's SID
                    2,                    // strings in lpszStrings
                    0,                    // no bytes of raw data
                    lpszStrings,          // array of error strings
                    NULL);                // no raw data

        (VOID) DeregisterEventSource(hEventSource);
    }
}




///////////////////////////////////////////////////////////////////
//
//  The following code handles service installation and removal
//
//
//  FUNCTION: CmdInstallService()
//
//  PURPOSE: Installs the service and Starts it
//
//  PARAMETERS:
//    argc: number of arguments
//	argv: all of the arguments including the program's name
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
void CmdInstallService(int argc, char **argv) {
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    TCHAR szPath[512];

    if ( GetModuleFileName( NULL, szPath, 512 ) == 0 ) {
        _tprintf(TEXT("Unable to install %s - %s\n"), TEXT(SZSERVICEDISPLAYNAME), GetLastErrorText(szErr, 256));
        return;
    }

    schSCManager = OpenSCManager(
                                NULL,                   // machine (NULL == local)
                                NULL,                   // database (NULL == default)
                                SC_MANAGER_ALL_ACCESS   // access required
                                );
    if ( schSCManager ) {
        schService = OpenService(schSCManager, TEXT(SZSERVICENAME), SERVICE_ALL_ACCESS);
        if ( !schService ) {
            schService = CreateService(
                                      schSCManager,               // SCManager database
                                      TEXT(SZSERVICENAME),        // name of service
                                      TEXT(SZSERVICEDISPLAYNAME), // name to display
                                      SERVICE_ALL_ACCESS,         // desired access
                                      SERVICE_WIN32_OWN_PROCESS,  // service type
                                      SERVICE_DEMAND_START,       // start type
                                      SERVICE_ERROR_NORMAL,       // error control type
                                      szPath,                     // service's binary
                                      NULL,                       // no load ordering group
                                      NULL,                       // no tag identifier
                                      TEXT(SZDEPENDENCIES),       // dependencies
                                      NULL,                       // LocalSystem account
                                      NULL);                      // no password
        } else {
            _tprintf(TEXT("%s already installed.\n"), TEXT(SZSERVICEDISPLAYNAME) );
        }
        if ( schService ) {
            if ( QueryServiceStatus( schService, &ssStatus ) ) {
                int rc;
                if ( ssStatus.dwCurrentState == SERVICE_STOPPED ) {
                    rc = StartService(schService, argc-1, argv+1);
                }


                if ( rc != 0 )
                    _tprintf(TEXT("%s started.\n"), TEXT(SZSERVICEDISPLAYNAME) );
            }

            CloseServiceHandle(schService);
        } else {
            _tprintf(TEXT("CreateService failed - %s\n"), GetLastErrorText(szErr, 256));
        }

        CloseServiceHandle(schSCManager);
    } else
        _tprintf(TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256));


}



//
//  FUNCTION: CmdRemoveService()
//
//  PURPOSE: Stops and removes the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE: service exists and is removed
//	FALSE: service doesn't exist
//
//  COMMENTS:
//
BOOL CmdRemoveService() {
    BOOL isExist = FALSE;
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    schSCManager = OpenSCManager(
                                NULL,                   // machine (NULL == local)
                                NULL,                   // database (NULL == default)
                                SC_MANAGER_ALL_ACCESS   // access required
                                );
    if ( schSCManager ) {
        schService = OpenService(schSCManager, TEXT(SZSERVICENAME), SERVICE_ALL_ACCESS);

        if ( schService ) {
            isExist = TRUE;
            // try to stop the service
            if ( ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) ) {
                _tprintf(TEXT("Stopping %s."), TEXT(SZSERVICEDISPLAYNAME));
                Sleep( 1000 );

                while ( QueryServiceStatus( schService, &ssStatus ) ) {
                    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING ) {
                        _tprintf(TEXT("."));
                        Sleep( 1000 );
                    } else
                        break;
                }

                if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
                    _tprintf(TEXT("\n%s stopped.\n"), TEXT(SZSERVICEDISPLAYNAME) );
                else
                    _tprintf(TEXT("\n%s failed to stop.\n"), TEXT(SZSERVICEDISPLAYNAME) );

            }

            // now remove the service
            if ( DeleteService(schService) )
                _tprintf(TEXT("%s removed.\n"), TEXT(SZSERVICEDISPLAYNAME) );
            else
                _tprintf(TEXT("DeleteService failed - %s\n"), GetLastErrorText(szErr,256));


            CloseServiceHandle(schService);
        }

        CloseServiceHandle(schSCManager);
    } else
        _tprintf(TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256));

    return isExist;
}

//
//  FUNCTION: CmdStartService()
//
//  PURPOSE: Start service if it exists
//
//  PARAMETERS:
//    argc: number of arguments
//	argv: arguments including program's name
//
//  RETURN VALUE:
//    TRUE: service exists and is started
//	FALSE: service doesn't exist
//
//  COMMENTS:
//
BOOL CmdStartService(int argc, char **argv) {
    BOOL isExist = FALSE;
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    schSCManager = OpenSCManager(
                                NULL,                   // machine (NULL == local)
                                NULL,                   // database (NULL == default)
                                SC_MANAGER_ALL_ACCESS   // access required
                                );
    if ( schSCManager ) {
        schService = OpenService(schSCManager, TEXT(SZSERVICENAME), SERVICE_ALL_ACCESS);

        if ( schService ) {
            isExist = TRUE;
            if ( QueryServiceStatus( schService, &ssStatus ) ) {
                int rc;
                if ( ssStatus.dwCurrentState == SERVICE_STOPPED ) {
                    rc = StartService(schService, argc-1, argv+1);
                }


                if ( rc != 0 )
                    _tprintf(TEXT("%s started.\n"), TEXT(SZSERVICEDISPLAYNAME) );
            }
            CloseServiceHandle(schService);            
        }
        CloseServiceHandle(schSCManager);
    } else
        _tprintf(TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256));

    return isExist;
}




//
//  FUNCTION: GetLastErrorText
//
//  PURPOSE: copies error message text to string
//
//  PARAMETERS:
//    lpszBuf - destination buffer
//    dwSize - size of buffer
//
//  RETURN VALUE:
//    destination buffer
//
//  COMMENTS:
//
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize ) {
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (LPTSTR)&lpszTemp,
                           0,
                           NULL );

    // supplied buffer is not long enough
    if ( !dwRet || ( (long)dwSize < (long)dwRet+14 ) )
        lpszBuf[0] = TEXT('\0');
    else {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  //remove cr and newline character
        _stprintf( lpszBuf, TEXT("%s (0x%x)"), lpszTemp, GetLastError() );
    }

    if ( lpszTemp )
        LocalFree((HLOCAL) lpszTemp );

    return lpszBuf;
}

/*--------------------------------------------------------------------
 * ServiceStart
 *
 * each time starting the service, this is the entry point of the service.
 * Start the service, certainly it is on server-mode
 * 
 *-------------------------------------------------------------------- */
VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv) {
    
    // report the status to the service control manager.
    //
    if ( !ReportStatusToSCMgr(
                             SERVICE_START_PENDING, // service state
                             NO_ERROR,              // exit code
                             3000) )                 // wait hint
        goto clean;

    thread_Settings* ext_gSettings = new thread_Settings;

    // Initialize settings to defaults
    Settings_Initialize( ext_gSettings );
    // read settings from environment variables
    Settings_ParseEnvironment( ext_gSettings );
    // read settings from command-line parameters
    Settings_ParseCommandLine( dwArgc, lpszArgv, ext_gSettings );

    // report the status to the service control manager.
    //
    if ( !ReportStatusToSCMgr(
                             SERVICE_START_PENDING, // service state
                             NO_ERROR,              // exit code
                             3000) )                 // wait hint
        goto clean;

    // if needed, redirect the output into a specified file
    if ( !isSTDOUT( ext_gSettings ) ) {
        redirect( ext_gSettings->mOutputFileName );
    }

    // report the status to the service control manager.
    //
    if ( !ReportStatusToSCMgr(
                             SERVICE_START_PENDING, // service state
                             NO_ERROR,              // exit code
                             3000) )                 // wait hint
        goto clean;
    
    // initialize client(s)
    if ( ext_gSettings->mThreadMode == kMode_Client ) {
        client_init( ext_gSettings );
    }

    // start up the reporter and client(s) or listener
    {
        thread_Settings *into = NULL;
#ifdef HAVE_THREAD
        Settings_Copy( ext_gSettings, &into );
        into->mThreadMode = kMode_Reporter;
        into->runNow = ext_gSettings;
#else
        into = ext_gSettings;
#endif
        thread_start( into );
    }
    
    // report the status to the service control manager.
    //
    if ( !ReportStatusToSCMgr(
                             SERVICE_RUNNING,       // service state
                             NO_ERROR,              // exit code
                             0) )                    // wait hint
        goto clean;

    clean:
    // wait for other (client, server) threads to complete
    thread_joinall();
}


//
//  FUNCTION: ServiceStop
//
//  PURPOSE: Stops the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    If a ServiceStop procedure is going to
//    take longer than 3 seconds to execute,
//    it should spawn a thread to execute the
//    stop code, and return.  Otherwise, the
//    ServiceControlManager will believe that
//    the service has stopped responding.
//    
VOID ServiceStop() {
#ifdef HAVE_THREAD
    Sig_Interupt( 1 );
#else
    sig_exit(1);
#endif
}

#endif
