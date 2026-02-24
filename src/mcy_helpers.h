#define WIN32_LEAN_AND_MEAN
#include <cstdio>
#include <cmath>
#include <Windows.h>

#pragma once
#define s8  int8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int64_t
#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define st size_t

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

template<typename T>
T Clamp(T value, T min, T max)
{
    if (value < min) 
        return min;
    if (value > max)
        return max;
    return value;
}

template<typename T>
T Max(T value1, T value2)
{
    if (value1 > value2) 
        return value1;
    return value2;
}

template<typename T>
T Min(T value1, T value2)
{
    if (value1 < value2) 
        return value1;
    return value2;
}

bool ReadEntireFile(const char* fileName, char** bufferOut, st* sizeOut)
{
    char errorBuffer[512];

    *bufferOut = nullptr;
    *sizeOut = 0;

    FILE* file = fopen(fileName, "rb");
    if (!file)
    {
        sprintf_s(errorBuffer, "Could not load file: %s\n", fileName);
        OutputDebugString(errorBuffer);
        return false;
    }
    
    std::fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize < 0)
    {
        sprintf_s(errorBuffer, "Invalid file size: %s\n", fileName);
        OutputDebugString(errorBuffer);
        std::fclose(file);
        return false;
    }
    *sizeOut = (st)fileSize;

    fseek(file, 0, SEEK_SET);
    *bufferOut = new char[*sizeOut];

    size_t bytesRead = std::fread(*bufferOut, 1, *sizeOut, file);
    std::fclose(file);

    if (bytesRead != *sizeOut)
    {
        delete[] *bufferOut;
        *bufferOut = nullptr;
        sizeOut = 0;
        sprintf_s(errorBuffer, "Could not read all bytes: %s\n", fileName);
        OutputDebugString(errorBuffer);
        return false; 
    }

    return true;
}