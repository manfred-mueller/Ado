#include "stdafx.h"

/*----------------------------------------------------------------------
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#define BANNER L"Ado - Administrator do, (C) 2019 - Manfred M\x81 \bller - http://www.nass-ek.de\nEin lokalisierter Fork von Elevate (https://github.com/jpassing/elevate).\n\n"


typedef struct _COMMAND_LINE_ARGS
{
	BOOL ShowHelp;
	BOOL Wait;
	BOOL StartComspec;
	PCWSTR ApplicationName;
	PCWSTR CommandLine;
} COMMAND_LINE_ARGS, *PCOMMAND_LINE_ARGS;

INT Launch(
	__in PCWSTR ApplicationName,
	__in PCWSTR CommandLine,
	__in BOOL Wait 
	)
{
	SHELLEXECUTEINFO Shex;
	ZeroMemory( &Shex, sizeof( SHELLEXECUTEINFO ) );
	Shex.cbSize = sizeof( SHELLEXECUTEINFO );
	Shex.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
	Shex.lpVerb = L"runas";
	Shex.lpFile = ApplicationName;
	Shex.lpParameters = CommandLine;
	Shex.nShow = SW_SHOWNORMAL;

	if ( ! ShellExecuteEx( &Shex ) )
	{
		DWORD Err = GetLastError();
		fwprintf( 
			stderr, 
			L"Laden von %s fehlgeschlagen: %d\n",
			ApplicationName,
			Err );
		return EXIT_FAILURE;
	}

	_ASSERTE( Shex.hProcess );
		
	if ( Wait )
	{
		WaitForSingleObject( Shex.hProcess, INFINITE );
	}
	CloseHandle( Shex.hProcess );
	return EXIT_SUCCESS;
}

INT DispatchCommand(
	__in PCOMMAND_LINE_ARGS Args 
	)
{
	WCHAR AppNameBuffer[ MAX_PATH ];
	WCHAR CmdLineBuffer[ MAX_PATH * 2 ];

	if ( Args->ShowHelp )
	{
		wprintf( 
			BANNER
			L"Einen Prozess auf der Befehlszeile mit erweiterten Rechten ausf\x81 \bhren\n"
			L"\n"
			L"Aufruf: Ado [-?|-wait|-k] prog [args]\n"
			L"-?    - Diese Hilfe anzeigen\n"
			L"-wait - Wartet auf Ende des Programms\n"
			L"-k    - Startet den %%COMSPEC%% Umgebungsvariablenwert und\n"
			L"	f\x81 \bhrt das Programm darin aus (CMD.EXE, 4NT.EXE, etc.)\n"
			L"prog  - Auszuf\x81 \bhrendes Programm\n"
			L"args  - Optionale Argumente f\x81 \br das auszuf\x81 \bhrende Programm\n" );

		return EXIT_SUCCESS;
	}

	if ( Args->StartComspec )
	{
		//
		// Resolve COMSPEC
		//
		if ( 0 == GetEnvironmentVariable( L"COMSPEC", AppNameBuffer, _countof( AppNameBuffer ) ) )
		{
			fwprintf( stderr, L"%%COMSPEC%% ist nicht definiert\n" );
			return EXIT_FAILURE;
		}
		Args->ApplicationName = AppNameBuffer;

		//
		// Prepend /K and quote arguments
		//
		if ( FAILED( StringCchPrintf(
			CmdLineBuffer,
			_countof( CmdLineBuffer ),
			L"/K \"%s\"",
			Args->CommandLine ) ) )
		{
			fwprintf( stderr, L"Befehlszeilenaufruf gescheitert\n" );
			return EXIT_FAILURE;
		}
		Args->CommandLine = CmdLineBuffer;
	}

	//wprintf( L"App: %s,\nCmd: %s\n", Args->ApplicationName, Args->CommandLine );
	return Launch( Args->ApplicationName, Args->CommandLine, Args->Wait );
}

int __cdecl wmain(
	__in int Argc, 
	__in WCHAR* Argv[]
	)
{
	OSVERSIONINFO OsVer;
	COMMAND_LINE_ARGS Args;
	INT Index;
	BOOL FlagsRead = FALSE;
	WCHAR CommandLineBuffer[ 260 ] = { 0 };

	ZeroMemory( &OsVer, sizeof( OSVERSIONINFO ) );
	OsVer.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	
	ZeroMemory( &Args, sizeof( COMMAND_LINE_ARGS ) );
	Args.CommandLine = CommandLineBuffer;

	//
	// Check OS version
	//
	if ( GetVersionEx( &OsVer ) &&
		OsVer.dwMajorVersion < 6 )
	{
		fwprintf( stderr, L"Dieses Werkzeug ist für Vista und höher gedacht.\n" );
		return EXIT_FAILURE;
	}

	//
	// Parse command line
	//
	for ( Index = 1; Index < Argc; Index++ )
	{
		if ( ! FlagsRead && 
			 ( Argv[ Index ][ 0 ] == L'-' || Argv[ Index ][ 0 ] == L'/' ) )
		{
			PCWSTR FlagName = &Argv[ Index ][ 1 ];
			if ( 0 == _wcsicmp( FlagName, L"?" ) )
			{
				Args.ShowHelp = TRUE;
			}
			else if ( 0 == _wcsicmp( FlagName, L"wait" ) )
			{
				Args.Wait = TRUE;
			}
			else if ( 0 == _wcsicmp( FlagName, L"k" ) )
			{
				Args.StartComspec = TRUE;
			}
			else
			{
				fwprintf( stderr, L"Unbekanntes Flag %s\n", FlagName );
				return EXIT_FAILURE;
			}
		}
		else
		{
			FlagsRead = TRUE;
			if ( Args.ApplicationName == NULL && ! Args.StartComspec )
			{
				Args.ApplicationName = Argv[ Index ];
			}
			else
			{
				if ( FAILED( StringCchCat(
						CommandLineBuffer,
						_countof( CommandLineBuffer ),
						Argv[ Index ] ) ) ||
					 FAILED( StringCchCat(
						CommandLineBuffer,
						_countof( CommandLineBuffer ),
						L" " ) ) )
				{
					fwprintf( stderr, L"Befehlszeile zu lang\n" );
					return EXIT_FAILURE;
				}
			}
		}
	}

#ifdef _DEBUG
	wprintf( 
		L"ShowHelp:        %s\n"
		L"Wait:            %s\n"
		L"StartComspec:    %s\n"
		L"ApplicationName: %s\n"
		L"CommandLine:     %s\n",
		Args.ShowHelp	  ? L"Y" : L"N",
		Args.Wait		  ?	L"Y" : L"N",
		Args.StartComspec ? L"Y" : L"N",
		Args.ApplicationName,
		Args.CommandLine );
#endif

	//
	// Validate args
	//
	if ( Argc <= 1 )
	{
		Args.ShowHelp = TRUE;
	}

	if ( ! Args.ShowHelp && 
		 ( (   Args.StartComspec && 0 == wcslen( Args.CommandLine ) ) ||
		   ( ! Args.StartComspec && Args.ApplicationName == NULL ) ) )
	{
		fwprintf( stderr, L"Ung\x81 \bltige Argumente\n" );
		return EXIT_FAILURE;
	}

	return DispatchCommand( &Args );
}

