@echo off
echo Building Task Scheduler Simulator...
if not exist scheduler_simulator.c (
    echo Error: scheduler_simulator.c not found!
    pause
    exit /b 1
)

echo Compiling...
gcc -Wall -Wextra -std=c99 -o scheduler_simulator.exe scheduler_simulator.c -luser32 -lkernel32

if %errorlevel% == 0 (
    echo Build successful!
    echo To run the program, execute: scheduler_simulator.exe
) else (
    echo Build failed!
)

pause