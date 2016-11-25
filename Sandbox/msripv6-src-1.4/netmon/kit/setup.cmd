@echo off

rem To handle filenames with spaces, which must be quoted by the user,
rem we avoid using quotes in the tests below.
if x%1 == x goto usage
if not exist %1 goto usage
if not exist %1\parsers goto usage

copy parser.ini %1
copy netmon.ini %1
copy mac.ini %1\parsers
copy tcpip.ini %1\parsers
copy tcpip6.ini %1\parsers
copy tcpip.dll %1\parsers
copy tcpip6.dll %1\parsers

goto end

:usage
echo.
echo usage: setup.cmd ^<directory^>
echo.
echo where ^<directory^> is where Network Monitor
echo is installed.
echo.

:end
