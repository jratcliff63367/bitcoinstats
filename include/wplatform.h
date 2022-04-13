#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Platform specific support functions.
// Currently just a string formatting function

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if ( x ) { x->release(); x = nullptr; }
#endif

namespace wplatform
{

bool getExePath(char* pathName, uint32_t maxPathSize);

int32_t stringFormat(char* dst, size_t dstSize, const char* format, ...)
#ifdef __GNUC__
    __attribute__((__format__(printf, 3, 4)))
#endif
    ;
int32_t stringFormatV(char* dst, size_t dstSize, const char* src, va_list arg);

// uses the high resolution timer to approximate a random number.
uint64_t getRandomTime(void);

// Normalize the path for windows vs. linux (on linux uses forward slashes, on linux uses back slashes)
void normalizePath(char* pathName);


void sleepNano(uint64_t nanoSeconds);

// Current memory usage
uint64_t getCurrentRSS(void);
// Peak memory usage
uint64_t getPeakRSS(void);

uint32_t getEpochTime(void); // current time_t value

int64_t atoi64(const char* str);

bool createDir(const char* pathName);

bool renameFile(const char* currentFileName, const char* newFileName);
bool deleteFile(const char* fileName);

}
