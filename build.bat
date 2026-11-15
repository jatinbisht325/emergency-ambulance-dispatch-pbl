@echo off
REM ==============================================================================
REM Emergency Ambulance Dispatch & Fleet Optimization System (GEHU Bhimtal)
REM Master Build & Compilation Automation Script
REM Author: Jatin Bisht (Team Leader)
REM ==============================================================================

echo ===============================================================================
echo     EMERGENCY AMBULANCE DISPATCH SYSTEM - MASTER BUILD PIPELINE (GCC/C99)
echo ===============================================================================

where gcc >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] GCC compiler was not found in your system PATH.
    echo Please make sure MinGW-w64 is installed and added to PATH.
    pause
    exit /b 1
)

echo [*] Compiling all team modules into unified binary: 'ambulance_system.exe'...
echo     - graph.c            (Jatin  : Adjacency List + Dijkstra)
echo     - dispatch_heap.c    (Kartik : Binary Min-Heap Priority Queue)
echo     - fleet_hospital.c   (Mansi  : Fleet Linked List + Hospital Bed Tracker)
echo     - call_system.c      (Mahima : FIFO Call Queue + LIFO History Stack)
echo     - main.c             (Master : Interactive CLI and Dispatch Loop)
echo.

gcc -Wall -Wextra -std=c99 -O2 graph.c dispatch_heap.c fleet_hospital.c call_system.c main.c -o ambulance_system.exe

if %errorlevel% neq 0 (
    echo.
    echo [FAIL] Master compilation failed with errors.
    pause
    exit /b 1
)

echo [SUCCESS] Build succeeded! Generated 'ambulance_system.exe' (0 warnings, 0 errors).
echo.
echo Launching Emergency Dispatch Control Center...
echo.
ambulance_system.exe
echo.
pause
