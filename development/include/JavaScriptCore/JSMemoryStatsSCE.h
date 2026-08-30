// Copyright (C) 2013 Sony Computer Entertainment Inc.
// All Rights Reserved.

#ifndef JSMemoryStatsSCE_h
#define JSMemoryStatsSCE_h

#include <JavaScriptCore/JSContextRef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JS_MEMORY_STATS_SCE_FLAG_GC        (1<<0)
#define JS_MEMORY_STATS_SCE_FLAG_EXECALLOC (1<<1)
#define JS_MEMORY_STATS_SCE_FLAG_ALL       ((1<<2) - 1)

struct JSMemoryStatsSCE {
    size_t flags;
    struct {
        size_t size;
        size_t capacity;
        size_t objectCount;
    } gcHeap;
    struct {
        size_t committed;
    } executableAllocator;
};

/*!
@function JSMemorystatsQuerySCE
@abstract Retrieve memory stats about a JS context
*/
JS_EXPORT void JSMemoryStatsQuerySCE(JSContextRef, JSMemoryStatsSCE &);

#ifdef __cplusplus
}
#endif

#endif /* JSMemoryStatsSCE_h */
