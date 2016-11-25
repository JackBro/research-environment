@echo off

if x"%COPYCMD%"==x"" set COPYCMD=/y

if x"%1" == x"" goto usage
if not exist "%1" goto usage

copy nt4.inf %1\oemsetup.inf
copy nt5.inf %1\netip6.inf
copy ..\license.txt %1
copy ..\license.htm %1
copy ..\ReadMe.htm %1
copy ..\tcpip6\common\obj\i386\%DDKBUILDENV%\tcpip6.sys %1
copy ..\wship6\obj\i386\%DDKBUILDENV%\wship6.dll %1
copy ..\ipv6\obj\i386\ipv6.exe %1
copy ..\ping6\obj\i386\ping6.exe %1
copy ..\tracert6\obj\i386\tracert6.exe %1
copy ..\ttcp\obj\i386\ttcp.exe %1
copy ..\mldtest\obj\i386\mldtest.exe %1
copy ..\hdrtest\obj\i386\hdrtest.exe %1
copy ..\ipsec\obj\i386\ipsec.exe %1
copy ..\ipsec\ipsecgui\ipsecgui.exe %1
copy ..\6to4cfg\obj\i386\6to4cfg.exe %1
copy ..\bin\i386\msafd.dll %1
copy ..\bin\i386\afd.sys %1
copy ..\bin\i386\afd.sp4 %1
copy ..\bin\i386\wininet.dll %1
copy ..\bin\i386\nslookup.exe %1

if not exist "%1\inc" mkdir "%1\inc"
copy ..\inc\*.h %1\inc

if not exist "%1\lib" mkdir "%1\lib"
copy ..\wship6\obj\i386\%DDKBUILDENV%\wship6.lib %1\lib

if not exist "%1\docs" mkdir "%1\docs"
xcopy /skr ..\docs\* %1\docs

cd ..\netmon\kit
if not exist "%1\netmon" mkdir "%1\netmon"
call .\mkkit.cmd "%1\netmon"
cd ..\..\kit

goto end

:usage
echo.
echo usage: mkkit.cmd ^<directory^>
echo.
echo where ^<directory^> is an existing directory
echo in which to place the completed kit.
echo.

:end
