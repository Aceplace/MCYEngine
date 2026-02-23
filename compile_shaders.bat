@echo off
setlocal

slangc.exe src/shader.slang -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o slang.spv

@REM slangc src\shader.slang ^
@REM     -target spirv ^
@REM     -profile spirv_1_3 ^
@REM     -emit-spirv-via-glsl ^
@REM     -fvk-use-entrypoint-name ^
@REM     -entry vertMain ^
@REM     -entry fragMain ^
@REM     -o slang.spv

@REM slangc src\shader.slang ^
@REM     -target spirv ^
@REM     -profile spirv_1_3 ^
@REM     -emit-spirv-via-glsl ^
@REM     -fvk-use-entrypoint-name ^
@REM     -entry vertMain ^
@REM     -o vert.spv

@REM if errorlevel 1 (
@REM     echo.
@REM     echo Shader compilation FAILED.
@REM     exit /b 1
@REM )

@REM slangc src\shader.slang ^
@REM     -target spirv ^
@REM     -profile spirv_1_3 ^
@REM     -emit-spirv-via-glsl ^
@REM     -fvk-use-entrypoint-name ^
@REM     -entry fragMain ^
@REM     -o frag.spv

@REM if errorlevel 1 (
@REM     echo.
@REM     echo Shader compilation FAILED.
@REM     exit /b 1
@REM )

@REM glslc.exe shader.vert -o vert.spv

@REM if errorlevel 1 (
@REM     echo.
@REM     echo Shader compilation FAILED.
@REM     exit /b 1
@REM )

@REM glslc.exe shader.frag -o frag.spv

@REM if errorlevel 1 (
@REM     echo.
@REM     echo Shader compilation FAILED.
@REM     exit /b 1
@REM )

echo.
echo Shader compilation succeeded
endlocal