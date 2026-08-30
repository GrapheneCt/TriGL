/*
	Vita Development Suite Libraries
*/

#ifndef _PSP2_TRILITHIUM_CORE_H_
#define _PSP2_TRILITHIUM_CORE_H_

#include <JavaScriptCore.h>

#ifdef __cplusplus
extern "C" {
#endif

void *fwMemoryMalloc(size_t size);
void fwMemoryFree(void *ptr);
void *fwMemoryMemalign(size_t align, size_t size);
void *fwMemoryMemset(void * ptr, int value, size_t num);

char *fwPathGet(int slot);

#ifdef __cplusplus
}
#endif

#endif /* _PSP2_TRILITHIUM_CORE_H_ */
