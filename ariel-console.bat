@echo off
REM Windows batch script to test Ariel with console output
echo Starting Ariel LV2 Host with console output...
echo.

REM Change to the directory where ariel.exe is located
cd /d "%~dp0"

REM Run Ariel with console output visible
ariel.exe.exe

REM Keep console open to see any output
echo.
echo Ariel has exited. Press any key to close this window.
pause > nul