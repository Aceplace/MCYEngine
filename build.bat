@echo off
cd /d %~dp0

echo ==========================
echo Setting up MSVC Environment
echo ==========================

if not defined VC_VARS_ALL_PATH (
    echo ERROR: VC_VARS_ALL_PATH environment variable not set!
    exit /b 1
)
call "%VC_VARS_ALL_PATH%"

echo ==========================
echo Building MCYEngine
echo ==========================

cl.exe ^
/Zi ^
/EHsc ^
/nologo ^
/Fe%CD%\mcy_engine.exe ^
%CD%\src\main.cpp ^
/I%VULKAN_SDK%\Include ^
/I%CD%\src\vendor ^
/link ^
/SUBSYSTEM:WINDOWS ^
/LIBPATH:%VULKAN_SDK%\Lib ^
vulkan-1.lib ^
user32.lib ^
gdi32.lib ^
kernel32.lib

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build FAILED
    exit /b %ERRORLEVEL%
)

echo.
echo Build succeeded.

echo ==========================
echo Cleaning temporary object files
echo ==========================

del /q *.obj 2>nul

echo Clean complete.