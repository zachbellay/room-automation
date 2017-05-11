@echo off

rem Command line arguments are either for IFTTT or ESP sites
rem First character must be a forward slash "/" for ESP HTTP Request
rem ESP Example: trigger.exe /LEDon
rem IFTTT Example: trigger.exe lamp_on
rem Multiple Request Example: trigger.exe lamp_on /LEDon
rem Supports up to 9 requests at a time

setlocal
	
	SET IFTTT_KEY="INSERT_IFFFTT_KEY_HERE"
	SET WIN_7_WGET_FILEPATH="C:\Program Files\GnuWin32\bin\wget.exe"
	SET WIN_10_WGET_FILEPATH="C:\Program Files (x86)\GnuWin32\bin\wget.exe"
	
	rem Gets Windows version and goes to corresponding function
	for /f "tokens=4-5 delims=. " %%i in ('ver') do set VERSION=%%i.%%j
	if "%version%" == "10.0" goto :WIN_10_START
	if "%version%" == "6.1" goto :WIN_7_START
	goto :eof
	
	:WIN_7_START
	IF [%1] == [] GOTO :eof

	SET temp=%1
	SET firstchar=%temp:~0,1%

	rem --no-check-certificate flag is to bypass HTTPS encryption
	rem -O NUL is to dump HTTP output file into NUL memory instead of harddrive
	if not "%firstchar%"=="/" goto :WIN_7_IFTTT
		%WIN_7_WGET_FILEPATH% --no-check-certificate -O NUL "192.168.1.6%1"
		shift
		goto :WIN_7_START
	
	
	:WIN_7_IFTTT
	%WIN_7_WGET_FILEPATH% --no-check-certificate -O NUL "https://maker.ifttt.com/trigger/"%1"/with/key/%IFTTT_KEY%"
	shift
	goto :WIN_7_START	
	
	
	:WIN_10_START
	IF [%1] == [] GOTO :eof

	SET temp=%1
	SET firstchar=%temp:~0,1%
	
	rem --no-check-certificate flag is to bypass HTTPS encryption
	rem -O NUL is to dump HTTP output file into NUL memory instead of harddrive
	if not "%firstchar%"=="/" goto :WIN_10_IFTTT
		%WIN_10_WGET_FILEPATH% --no-check-certificate -O NUL "192.168.1.6%1"
		shift
		goto :WIN_10_START

	:WIN_10_IFTTT
	%WIN_10_WGET_FILEPATH% --no-check-certificate -O NUL "https://maker.ifttt.com/trigger/"%1"/with/key/%IFTTT_KEY%"
	shift
	goto :WIN_10_START
	
endlocal