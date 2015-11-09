REM batch file to test the unit test for OpenDungeons
REM The boost test filenames are supposed to be as follows:
REM boosttest-source_tests-LL-*.exe
REM LL=name of the level server the game should launch (00 if none). For example, if gg, a server instance will be launched with test map gg.level
REM * can be any relevant description
@echo off

SET OPEN_DUNGEONS_EXE=OpenDungeons.exe

for %%i in (boosttest-source_tests-*) do call:handleBoostTest %%i
goto:eof

:handleBoostTest
set "boost_test=%~1"
echo %boost_test%
set "level=%boost_test:~23,2%"
if NOT "%level%" == "00" (
echo launching a server with map %level%.level
start %OPEN_DUNGEONS_EXE% --server %level%.level --port 32222 --log srvLog.txt
) else (
echo no need to launch a server
)
%boost_test%
if %ERRORLEVEL% NEQ 0 goto ERROR
goto:eof

:OK
echo TESTS OK
exit 0

:ERROR
echo ERROR DURING TESTS
exit %ERRORLEVEL%
