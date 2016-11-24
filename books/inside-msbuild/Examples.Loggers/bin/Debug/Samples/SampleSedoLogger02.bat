ECHO OFF
ECHO SedoLogger Sample
ECHO Assumes the existance of a project at '..\..\..\..\unittest\Unittest.Proj1\Unittest.Proj1.csproj'
ECHO Assumes that msbuild.exe is on the path
ECHO ON
msbuild.exe ..\..\..\..\unittest\Unittest.Proj1\Unittest.Proj1.csproj -l:XmlLogger,..\Examples.Loggers.dll;v=diag;logfile=log.xml




