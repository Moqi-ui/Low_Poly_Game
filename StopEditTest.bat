@echo off
title Blaster DS Local Test - Stop

echo Stopping all test processes ...

taskkill /im "UnrealEditor-Cmd.exe" /f >nul 2>&1
taskkill /im "UnrealEditor-Win64-DebugGame.exe" /f >nul 2>&1

echo Done. All test processes stopped.
pause
