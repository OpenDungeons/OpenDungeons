REM batch file to test the unit test for OpenDungeons
SET OPEN_DUNGEONS_EXE=OpenDungeons.exe
boosttest-source_tests-ConsoleInterface.exe
if %ERRORLEVEL% NEQ 0 goto ERROR
boosttest-source_tests-Random.exe
if %ERRORLEVEL% NEQ 0 goto ERROR
boosttest-source_tests-Goal.exe
if %ERRORLEVEL% NEQ 0 goto ERROR
boosttest-source_tests-ODPacket.exe
if %ERRORLEVEL% NEQ 0 goto ERROR
boosttest-source_tests-Pathfinding.exe
if %ERRORLEVEL% NEQ 0 goto ERROR
start %OPEN_DUNGEONS_EXE% --server UnitTest.level --port 32222 --log srvLog.txt
boosttest-source_tests-LaunchGame.exe
if %ERRORLEVEL% NEQ 0 goto ERROR

:OK
echo TESTS OK
exit 0

:ERROR
echo ERROR DURING TESTS
exit %ERRORLEVEL%
