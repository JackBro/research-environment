@echo off

if x"%1" == x"" goto usage
if not exist "%1" goto usage

copy ..\..\license.txt %1
copy ..\..\license.htm %1
copy ..\ReadMe.txt %1
copy setup.cmd %1
copy parser.ini %1
copy netmon.ini %1
copy mac.ini %1
copy tcpip.ini %1
copy tcpip6.ini %1
copy ..\bin\i386\tcpip.dll %1
copy ..\tcpip6\obj\i386\%DDKBUILDENV%\tcpip6.dll %1

goto end

:usage
echo.
echo usage: mkkit.cmd ^<directory^>
echo.
echo where ^<directory^> is an existing directory
echo in which to place the completed kit.
echo.

:end
