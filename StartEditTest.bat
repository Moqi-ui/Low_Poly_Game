@echo off
title Blaster DS Local Test (Editor Dev)

set ENGINE=d:\UnrealEngine51\Engine\Binaries\Win64
set PROJECT=f:\00_Imrcao\01_MyProject\Low_Poly_Game\Blaster.uproject
set MAP=/Game/Maps/Lobby?listen

echo [1/3] Starting DS ...
start "Blaster DS" /min "%ENGINE%\UnrealEditor-Cmd.exe" "%PROJECT%" %MAP% -server -log

echo [2/3] Waiting for DS (3s) ...
timeout /t 3 /nobreak >nul

echo [3/3] Starting Client 1 and Client 2 ...
start "Client 1" "%ENGINE%\UnrealEditor.exe" "%PROJECT%" 127.0.0.1 -game -windowed -ResX=960 -ResY=540
timeout /t 1 /nobreak >nul
start "Client 2" "%ENGINE%\UnrealEditor.exe" "%PROJECT%" 127.0.0.1 -game -windowed -ResX=640 -ResY=480

echo Done. Close clients and server manually.
pause
exit
