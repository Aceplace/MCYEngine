@echo off
setlocal

slangc src\shader.slang ^
    -target spirv ^
    -profile spirv_1_3 ^
    -emit-spirv-via-glsl ^
    -fvk-use-entrypoint-name ^
    -entry vertMain ^
    -entry fragMain ^
    -o slang.spv

glslc.exe shader.vert -o vert.spv

if errorlevel 1 (
    echo.
    echo Shader compilation FAILED.
    exit /b 1
)

glslc.exe shader.frag -o frag.spv

if errorlevel 1 (
    echo.
    echo Shader compilation FAILED.
    exit /b 1
)

echo.
echo Shader compilation succeeded
endlocal