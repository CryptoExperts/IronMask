#pragma once

// Size of batches when batching is required. For now, this is only
// used for RPE2 verification. (it should be used for RPE12 and RPE21
// as well though; TODO)
#define BATCH_SIZE 1000000 // 1 million

#include <stdint.h>
#include <sys/sysinfo.h>

#define LARGE_CIRCUITS
// To support circuits with more than 255 variables, define LARGE_CIRCUITS
#ifdef LARGE_CIRCUITS
#define VAR_TYPE uint16_t
#else
#define VAR_TYPE uint8_t
#endif

// By default, VRAPS supports up to 31 shares. To support up to 63
// shares, you can define VERY_HIGH_ORDER. If you want to optimize
// your VRAPS for 7 (resp. 15) shares or less, define LOW_ORDER
// (resp. MEDIUM_ORDER).
#ifdef VERY_HIGH_ORDER
#define DEPENDENCY_TYPE uint64_t
#elif defined(MEDIUM_ORDER)
#define DEPENDENCY_TYPE uint16_t
#elif defined(LOW_ORDER)
#define DEPENDENCY_TYPE uint8_t
#else
#define DEPENDENCY_TYPE uint32_t
#endif


// Number of cores to use when the "-j -1" option is used
#define CORES_TO_USE_FOR_MULTITHREADING (get_nprocs() - 1)
