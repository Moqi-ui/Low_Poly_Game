@echo off

echo =====================================
echo Unreal Engine Project Clean Tool
echo =====================================

set PROJECT_DIR=%cd%

echo.
echo Cleaning Intermediate...
if exist "%PROJECT_DIR%\Intermediate" (
    rmdir /s /q "%PROJECT_DIR%\Intermediate"
)

echo Cleaning Binaries...
if exist "%PROJECT_DIR%\Binaries" (
    rmdir /s /q "%PROJECT_DIR%\Binaries"
)

echo Cleaning DerivedDataCache...
if exist "%PROJECT_DIR%\DerivedDataCache" (
    rmdir /s /q "%PROJECT_DIR%\DerivedDataCache"
)

echo Cleaning Saved...
if exist "%PROJECT_DIR%\Saved" (
    rmdir /s /q "%PROJECT_DIR%\Saved"
)

echo Cleaning .vs folder...
if exist "%PROJECT_DIR%\.vs" (
    rmdir /s /q "%PROJECT_DIR%\.vs"
)

echo Cleaning Visual Studio solution...
del /q "%PROJECT_DIR%\*.sln"

echo Cleaning Visual Studio project files...
del /q "%PROJECT_DIR%\*.vcxproj*"

echo.
echo Clean Completed!
echo You should regenerate project files now.

pause