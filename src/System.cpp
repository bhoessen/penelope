/***************************************************************************************[System.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "penelope/utils/System.h"

#if defined(__linux__)

#include <stdio.h>
#include <stdlib.h>
#include <sstream>

using namespace penelope;

// TODO: split the memory reading functions into two: one for reading high-watermark of RSS, and
// one for reading the current virtual memory size.

static inline int memReadStat(int field)
{
    pid_t pid = getpid();
    int   value;

    std::stringstream nameBuf;
    nameBuf << "/proc/" << pid << "/statm";

    FILE* in = fopen(nameBuf.str().c_str(), "rb");
    if (in == NULL) return 0;

    for (; field >= 0; field--)
        if (fscanf(in, "%50d", &value) != 1)
            printf("ERROR! Failed to parse memory statistics from \"/proc\".\n"), exit(1);
    fclose(in);
    return value;
}


static inline int memReadPeak(void)
{
    
    pid_t pid = getpid();

    std::stringstream nameBuf;
    nameBuf << "/proc/" << pid << "/statm";
    FILE* in = fopen(nameBuf.str().c_str(), "rb");
    if (in == NULL) return 0;

    // Find the correct line, beginning with "VmPeak:":
    int peak_kb = 0;
    while (!feof(in) && fscanf(in, "VmPeak: %50d kB", &peak_kb) != 1)
        while (!feof(in) && fgetc(in) != '\n')
            ;
    fclose(in);

    return peak_kb;
}

double penelope::memUsed() { return (double)memReadStat(0) * (double)getpagesize() / (1024*1024); }
double penelope::memUsedPeak() {
    double peak = memReadPeak() / 1024;
    return peak == 0 ? memUsed() : peak; }

#elif defined(__FreeBSD__)

double penelope::memUsed(void) {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (double)ru.ru_maxrss / 1024; }
double penelope::memUsedPeak(void) { return memUsed(); }

#elif defined(_MSC_VER) || defined(__MINGW32__)

//This windows code was contributed by Axel Kemper. Many thanks to him!

#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#include <time.h>

double penelope::memUsed() {
    HANDLE hProcess = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;
    double size = 0;

    if (!GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        size = pmc.WorkingSetSize / (1024.0 * 1024.0);
    }

    return size;
}

double penelope::memUsedPeak() {
    HANDLE hProcess = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;
    double size = 0;

    if (!GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        size = pmc.PeakWorkingSetSize / (1024.0 * 1024.0);
    }

    return size;
}

#elif defined(__APPLE__)
#include <malloc/malloc.h>

double penelope::memUsed(void) {
    malloc_statistics_t t;
    malloc_zone_statistics(NULL, &t);
    return (double)t.max_size_in_use / (1024*1024); }
double penelope::memUsedPeak(void) { return memUsed(); }
#else
double penelope::memUsed() {
    return 0; }

double penelope::memUsedPeak(void) { return memUsed(); }
#endif
