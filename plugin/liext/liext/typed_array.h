#pragma once

#include <JavaScriptCore.h>

typedef struct LIExtTypedArrayPriv {
	void *data;
	size_t size;
	size_t count;
} LIExtTypedArrayPriv;

void liextCreateClassTypedArray(JSContextRef ctx, JSValueRef *excp);
void liextReleaseClassTypedArray(JSContextRef ctx, JSValueRef *excp);
JSObjectRef Int8ArrayNew(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
