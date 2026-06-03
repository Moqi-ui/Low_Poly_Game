@echo off
echo ==========================================
echo UE5 VSCode Auto Setup Tool
echo ==========================================

set UE_PATH=C:\UnrealEngine-5.7.4-release
set UBT="%UE_PATH%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"

set PROJECT_DIR=%cd%

for %%i in (*.uproject) do (
    set PROJECT_FILE=%%i
    set PROJECT_NAME=%%~ni
)

if "%PROJECT_FILE%"=="" (
    echo ERROR: No .uproject file found
    pause
    exit
)

echo Found Project: %PROJECT_FILE%

echo.
echo Generating VSCode project files...
%UBT% -projectfiles -project="%PROJECT_DIR%\%PROJECT_FILE%" -game -engine -VSCode

pause