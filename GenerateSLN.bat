@echo off

echo ======================================
echo Unreal Engine Generate Visual Studio Solution
echo ======================================

REM Unreal Engine 路径（修改为你的 UE 路径）
set UE_PATH=C:\UnrealEngine-5.7.4-release

REM 当前目录
set PROJECT_DIR=%cd%

REM 查找 .uproject
for %%i in (*.uproject) do (
    set PROJECT_FILE=%%i
)

if "%PROJECT_FILE%"=="" (
    echo ERROR: No .uproject file found in this folder.
    pause
    exit
)

echo Found Project: %PROJECT_FILE%
echo.

echo Generating Visual Studio solution...

"%UE_PATH%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" ^
-projectfiles ^
-project="%PROJECT_DIR%\%PROJECT_FILE%" ^
-game ^
-engine ^
-2022

echo.
echo Solution generated successfully.

pause