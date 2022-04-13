#include "wplatform.h"
#include <chrono>
#include <stdarg.h>
#include <stdio.h>
#include <thread>
#include <time.h>

#define CARB_PLATFORM_WINDOWS 1

#if CARB_PLATFORM_WINDOWS
    #include <windows.h>
    #include <Psapi.h>
    #include <direct.h>
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <signal.h>
    #include <sys/stat.h>
#endif


/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#    include <psapi.h>
#    include <windows.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#    include <sys/resource.h>

#    include <unistd.h>

#    if defined(__APPLE__) && defined(__MACH__)
#        include <mach/mach.h>

#    elif (defined(_AIX) || defined(__TOS__AIX__)) ||                                                                  \
        (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#        include <fcntl.h>
#        include <procfs.h>

#    elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#        include <stdio.h>

#    endif

#else
#    error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif


namespace wplatform
{

bool getExePath(char* pathName, uint32_t maxPathSize)
{
    bool ret = false;

#if CARB_PLATFORM_WINDOWS
    GetModuleFileNameA(nullptr, pathName, maxPathSize);
    ret = true;
#else
    ssize_t len = readlink("/proc/self/exe", pathName, maxPathSize - 1);
    if (len > 0)
    {
        pathName[len] = '\0';
        ret = true;
    }
    else
    {
        ret = false;
    }
#endif
    // Zero byte terminate the last slash character since we are returning the path, not the full
    // executable name
    char* scan = pathName;
    char* lastSlash = nullptr;
    while (*scan)
    {
        if (*scan == '/' || *scan == '\\')
        {
            lastSlash = scan;
        }
        scan++;
    }
    if (lastSlash)
    {
        *lastSlash = 0;
    }

    return ret;
}


int32_t stringFormatV(char* dst, size_t dstSize, const char* src, va_list arg)
{
    return ::vsnprintf(dst, dstSize, src, arg);
}

int32_t stringFormat(char* dst, size_t dstSize, const char* format, ...)
{
    va_list arg;
    va_start(arg, format);
    int32_t r = stringFormatV(dst, dstSize, format, arg);
    va_end(arg);
    return r;
}

// uses the high resolution timer to approximate a random number.
uint64_t getRandomTime(void)
{
    uint64_t seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return seed;
}

void sleepNano(uint64_t nanoSeconds)
{
    std::this_thread::sleep_for(std::chrono::nanoseconds(nanoSeconds)); // s
}

void normalizePath(char* pathName)
{
#if CARB_PLATFORM_WINDOWS
    while (*pathName)
    {
        if (*pathName == '/')
        {
            *pathName = '\\';
        }
        pathName++;
    }
#else
    while (*pathName)
    {
        if (*pathName == '\\')
        {
            *pathName = '/';
        }
        pathName++;
    }
#endif
}


/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
uint64_t getPeakRSS()
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return (size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) ||                                                                      \
    (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
    /* AIX and Solaris ------------------------------------------ */
    struct psinfo psinfo;
    int fd = -1;
    if ((fd = open("/proc/self/psinfo", O_RDONLY)) == -1)
        return (size_t)0L; /* Can't open? */
    if (read(fd, &psinfo, sizeof(psinfo)) != sizeof(psinfo))
    {
        close(fd);
        return (size_t)0L; /* Can't read? */
    }
    close(fd);
    return (size_t)(psinfo.pr_rssize * 1024L);

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
    /* BSD, Linux, and OSX -------------------------------------- */
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
#    if defined(__APPLE__) && defined(__MACH__)
    return (size_t)rusage.ru_maxrss;
#    else
    return static_cast<size_t>(rusage.ru_maxrss * 1024L);
#    endif

#else
    /* Unknown OS ----------------------------------------------- */
    return (size_t)0L; /* Unsupported. */
#endif
}


/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
uint64_t getCurrentRSS()
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
    /* OSX ------------------------------------------------------ */
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) != KERN_SUCCESS)
        return (size_t)0L; /* Can't access? */
    return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
    /* Linux ---------------------------------------------------- */
    long rss = 0L;
    FILE* fp = NULL;
    if ((fp = fopen("/proc/self/statm", "r")) == NULL)
        return 0; /* Can't open? */
    if (fscanf(fp, "%*s%ld", &rss) != 1)
    {
        fclose(fp);
        return 0; /* Can't read? */
    }
    fclose(fp);
    return static_cast<size_t>(rss) * static_cast<size_t>(sysconf(_SC_PAGESIZE));

#else
    /* AIX, BSD, Solaris, and Unknown OS ------------------------ */
    return (size_t)0L; /* Unsupported. */
#endif
}


uint32_t getEpochTime(void) // current time_t value
{
    time_t t;
    time(&t);
    return uint32_t(t);
}


int64_t atoi64(const char* str)
{
    int64_t ret = 0;

    if (str)
    {
#if CARB_PLATFORM_WINDOWS
        ret = _atoi64(str);
#else
        ret = strtoul(str, nullptr, 10);
#endif
    }

    return ret;
}

bool createDir(const char* pathName)
{
    bool ret = false;

#if CARB_PLATFORM_WINDOWS
    int err = _mkdir(pathName);
    ret = err ? false : true;
#else
    const int dir_err = mkdir(pathName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (-1 == dir_err)
    {
        ret = false;
    }
    else
    {
        ret = true;
    }
#endif

    return ret;
}

bool renameFile(const char* currentFileName, const char* newFileName)
{
    bool ret = false;

    int err = rename(currentFileName, newFileName);
    ret = err ? false : true;

    return ret;
}

bool deleteFile(const char* fileName)
{
    bool ret = false;

    int err = remove(fileName);
    ret = err ? false : true;

    return ret;
}

}
