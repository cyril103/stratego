@echo off
cd /d "%~dp0"
if exist "build\stratego.exe" (
    start "" "build\stratego.exe"
) else if exist "build\Release\stratego.exe" (
    start "" "build\Release\stratego.exe"
) else (
    echo Compilez le projet avec CMake. Instructions dans README.md.
    pause
)
