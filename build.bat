@echo off
REM ==============================================================================
REM Emergency Ambulance Dispatch & Fleet Optimization System
REM Core Engine Build & Execution Script
REM Author: Jatin (Team Leader)
REM ==============================================================================

echo [BUILD] Compiling Jatin's Graph & Routing Engine...

where gcc >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] GCC compiler not found in PATH.
    echo Please make sure MinGW / GCC is installed and added to PATH.
    pause
    exit /b 1
)

gcc -Wall -Wextra -std=c99 -O2 graph.c test_graph.c -o test_graph.exe

if %errorlevel% neq 0 (
    echo [FAIL] Compilation failed with errors.
    pause
    exit /b 1
)

echo [SUCCESS] Compilation successful! Generated 'test_graph.exe'.
echo [RUN] Running simulation...
echo.
test_graph.exe
echo.
pause
