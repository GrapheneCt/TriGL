#include <kernel.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <JavaScriptCore.h>

#include "typed_array.h"

#define SET_FUNCTION_PROPERTY(str) \
name = JSStringCreateWithUTF8CString(#str); \
prop = JSObjectMakeFunctionWithCallback(ctx, NULL, liext_##str); \
JSObjectSetProperty(ctx, obj, name, prop, 0, excp); \
JSStringRelease(name);

#define DELETE_FUNCTION_PROPERTY(str) \
name = JSStringCreateWithUTF8CString(#str); \
JSObjectDeleteProperty(ctx, obj, name, excp); \
JSStringRelease(name);

#define V2N(x) JSValueToNumber(ctx, x, exception)
#define N2V(x) JSValueMakeNumber(ctx, (double)((int64_t)x))

#define LIEXT_FUNCTION_HEAD(str) JSValueRef liext_##str(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)

LIEXT_FUNCTION_HEAD(ioOpen)
{
	char *buffer = NULL;
	size_t sz = 0;

	JSStringRef str = JSValueToStringCopy(ctx, arguments[0], exception);
	sz = JSStringGetMaximumUTF8CStringSize(str);
	buffer = malloc(sz);
	JSStringGetUTF8CString(str, buffer, sz);
	JSStringRelease(str);

	SceUID ret = sceIoOpen(buffer, (int)V2N(arguments[1]), (SceIoMode)V2N(arguments[2]));
	free(buffer);

	if (ret <= 0)
		return JSValueMakeUndefined(ctx);

	return N2V(ret);
}

LIEXT_FUNCTION_HEAD(ioRead)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[1], exception));

	return N2V(sceIoPread((SceUID)V2N(arguments[0]), priv->data, (SceSize)V2N(arguments[2]), (SceOff)V2N(arguments[3])));
}

LIEXT_FUNCTION_HEAD(ioWrite)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[1], exception));

	return N2V(sceIoPwrite((SceUID)V2N(arguments[0]), priv->data, (SceSize)V2N(arguments[2]), (SceOff)V2N(arguments[3])));
}

LIEXT_FUNCTION_HEAD(ioClose)
{
	return N2V(sceIoClose((SceUID)V2N(arguments[0])));
}

LIEXT_FUNCTION_HEAD(ioSeek)
{
	return N2V(sceIoLseek((SceUID)V2N(arguments[0]), (SceOff)V2N(arguments[1]), (int)V2N(arguments[2])));
}

LIEXT_FUNCTION_HEAD(ioReadFile)
{
	SceIoStat st;
	char *buffer = NULL;
	size_t sz = 0;
	SceUID fd = 0;

	JSStringRef str = JSValueToStringCopy(ctx, arguments[0], exception);
	sz = JSStringGetMaximumUTF8CStringSize(str);
	buffer = malloc(sz);
	JSStringGetUTF8CString(str, buffer, sz);
	JSStringRelease(str);

	if (sceIoGetstat(buffer, &st) < 0) {
		free(buffer);
		return JSValueMakeNull(ctx);
	}

	fd = sceIoOpen(buffer, SCE_O_RDONLY, 0);
	free(buffer);

	if (fd <= 0) {
		return JSValueMakeNull(ctx);
	}

	JSValueRef jsz = N2V(st.st_size);
	JSObjectRef ret = Int8ArrayNew(ctx, NULL, 1, &jsz, exception);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(ret);
	
	if (priv == NULL) {
		return JSValueMakeNull(ctx);
	}

	sceIoRead(fd, priv->data, (SceSize)st.st_size);
	sceIoClose(fd);

	return ret;
}

LIEXT_FUNCTION_HEAD(kernelGetModel)
{
	return N2V(sceKernelGetModel());
}

void liextAddFunctionProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp)
{
	JSStringRef name;
	JSObjectRef prop;

	SET_FUNCTION_PROPERTY(ioOpen)
	SET_FUNCTION_PROPERTY(ioRead)
	SET_FUNCTION_PROPERTY(ioWrite)
	SET_FUNCTION_PROPERTY(ioClose)
	SET_FUNCTION_PROPERTY(ioSeek)
	SET_FUNCTION_PROPERTY(ioReadFile)
	SET_FUNCTION_PROPERTY(kernelGetModel)
}

void liextRemoveFunctionProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp)
{
	JSStringRef name;

	DELETE_FUNCTION_PROPERTY(ioOpen)
	DELETE_FUNCTION_PROPERTY(ioRead)
	DELETE_FUNCTION_PROPERTY(ioWrite)
	DELETE_FUNCTION_PROPERTY(ioClose)
	DELETE_FUNCTION_PROPERTY(ioSeek)
	DELETE_FUNCTION_PROPERTY(ioReadFile)
	DELETE_FUNCTION_PROPERTY(kernelGetModel)
}