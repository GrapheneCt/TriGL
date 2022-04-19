#include <kernel.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <JavaScriptCore.h>

#include "typed_array.h"

#define V2N(x) JSValueToNumber(ctx, x, exception)
#define N2V(x) JSValueMakeNumber(ctx, (double)((int64_t)x))

static JSClassRef sdef_Int8Array;
static JSClassRef sdef_Uint8Array;
static JSClassRef sdef_Int16Array;
static JSClassRef sdef_Uint16Array;
static JSClassRef sdef_Int32Array;
static JSClassRef sdef_Uint32Array;
static JSClassRef sdef_Float32Array;

unsigned int calculateIndex(int16_t *buf, size_t len)
{
	if (len > 4)
		return 0;

	switch (len) {
	case 1:
		return buf[0] - 0x30;
		break;
	case 2:
		return (buf[1] - 0x30) + (buf[0] - 0x30) * 10;
		break;
	case 3:
		return (buf[2] - 0x30) + (buf[1] - 0x30) * 10 + (buf[0] - 0x30) * 100;
		break;
	case 4:
		return (buf[3] - 0x30) + (buf[2] - 0x30) * 10 + (buf[1] - 0x30) * 100 + (buf[0] - 0x30) * 1000;
		break;
	}

	return 0;
}

void ArrayDelete(JSObjectRef object)
{
	LIExtTypedArrayPriv *p = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);
	if (p) {
		if (p->data)
			free(p->data);
		free(p);
	}
}

JSValueRef ArrayCopy(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	size_t bytesToCopy = 0;
	size_t bytesStart = 0;

	if (argumentCount > 3)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *srcPriv = JSObjectGetPrivate(thisObject);
	LIExtTypedArrayPriv *dstPriv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[0], exception));
	size_t dataTypeSize = (srcPriv->size / srcPriv->count);

	if (argumentCount == 1) {
		bytesToCopy = srcPriv->size;
	}
	else if (argumentCount == 3) {
		LIExtTypedArrayPriv *srcPriv = JSObjectGetPrivate(thisObject);
		LIExtTypedArrayPriv *dstPriv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[0], exception));
		size_t startCount = (size_t)V2N(arguments[1]);
		size_t toCopyCount = (size_t)V2N(arguments[2]);

		bytesStart = startCount * dataTypeSize;
		bytesToCopy = toCopyCount * dataTypeSize;
		if (bytesStart + bytesToCopy > srcPriv->size)
			return JSValueMakeUndefined(ctx);
	}
	else
		return JSValueMakeUndefined(ctx);

	if (bytesToCopy > dstPriv->size)
		bytesToCopy = dstPriv->size;

	memcpy(dstPriv->data, srcPriv->data + bytesStart, bytesToCopy);


	return N2V((bytesToCopy / dataTypeSize));
}

#define TYPE	int8_t

JSValueRef Int8ArrayRead(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 1)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	return N2V(p[(size_t)V2N(arguments[0])]);
}

JSValueRef Int8ArrayWrite(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	TYPE toWrite = (TYPE)V2N(arguments[0]);
	p[(size_t)V2N(arguments[1])] = toWrite;

	return arguments[0];
}

JSValueRef Int8ArrayToArray(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSValueRef jlen;
	JSValueRef *val;
	JSStringRef jls;
	TYPE *p;
	size_t count = 0;

	if (argumentCount > 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	p = (TYPE *)priv->data;
	count = priv->count;

	if (argumentCount == 0) {

		val = malloc(count * sizeof(JSValueRef));

		for (int i = 0; i < count; i++) {
			val[i] = N2V(p[i]);
		}

		ret = JSObjectMakeArray(ctx, count, val, exception);
	}
	else if (argumentCount == 1) {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t j = 0;
		if (offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(count * sizeof(JSValueRef));

		for (int i = offset; i < count; i++) {
			val[j] = N2V(p[i]);
			j++;
		}

		ret = JSObjectMakeArray(ctx, count - offset, val, exception);
	}
	else {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t toCopy = (size_t)V2N(arguments[1]);
		size_t j = 0;
		count -= offset;
		if (toCopy > count || offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(toCopy * sizeof(JSValueRef));

		for (int i = offset; ; i++) {
			val[j] = N2V(p[i]);
			if (j == toCopy)
				break;
			j++;
		}

		ret = JSObjectMakeArray(ctx, toCopy, val, exception);
	}

	free(val);
	return ret;
}

JSObjectRef Int8ArrayNew(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSStringRef jls;
	JSObjectRef prop;
	size_t count = 0;
	void *p = NULL;

	if (argumentCount == 0 || argumentCount > 1)
		return JSValueToObject(ctx, N2V(0), exception);

	JSType type = JSValueGetType(ctx, arguments[0]);
	if (type == kJSTypeNumber) {
		count = (size_t)V2N(arguments[0]);
		p = calloc(count, sizeof(TYPE));
	}
	else if (type == kJSTypeObject) {
		JSObjectRef jarr;
		JSValueRef jlen;
		JSValueRef pres;
		TYPE *pval;

		jarr = JSValueToObject(ctx, arguments[0], exception);
		jls = JSStringCreateWithUTF8CString("length");
		jlen = JSObjectGetProperty(ctx, jarr, jls, exception);
		JSStringRelease(jls);
		count = (size_t)V2N(jlen);
		p = calloc(count, sizeof(TYPE));
		pval = (TYPE *)p;

		for (int i = 0; i < count; i++) {
			pres = JSObjectGetPropertyAtIndex(ctx, jarr, i, exception);
			pval[i] = (TYPE)V2N(pres);
		}
	}
	else {
		return JSValueToObject(ctx, N2V(0), exception);
	}

	LIExtTypedArrayPriv *priv = malloc(sizeof(LIExtTypedArrayPriv));
	priv->data = p;
	priv->size = count * sizeof(TYPE);
	priv->count = count;
	ret = JSObjectMake(ctx, sdef_Int8Array, priv);

	jls = JSStringCreateWithUTF8CString("length");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->count), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	jls = JSStringCreateWithUTF8CString("byteLength");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->size), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	return ret;
}

JSValueRef Int8ArrayGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] == 0x6C) { //length
		return N2V(priv->count);
	}
	else if (buf[0] == 0x62) { //byteLength
		return N2V(priv->size);
	}
	else if ((buf[0] >= 0x30 && buf[0] <= 0x39)) {
		TYPE *dptr = (TYPE *)priv->data;
		return N2V(dptr[calculateIndex(buf, len)]);
	}

	return NULL;
}

bool Int8ArraySetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] < 0x30 || buf[0] > 0x39)
		return false;

	TYPE *dptr = (TYPE *)priv->data;
	dptr[calculateIndex(buf, len)] = (TYPE)V2N(value);

	return true;
}

static JSStaticFunction Int8ArrayFuncs[] = {
	{ "read", Int8ArrayRead, kJSPropertyAttributeNone },
	{ "write", Int8ArrayWrite, kJSPropertyAttributeNone },
	{ "toArray", Int8ArrayToArray, kJSPropertyAttributeNone },
	{ "copy", ArrayCopy, kJSPropertyAttributeNone },
	{ 0, 0, 0, 0 }
};

#undef TYPE

#define TYPE	uint8_t

JSValueRef Uint8ArrayRead(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 1)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	return N2V(p[(size_t)V2N(arguments[0])]);
}

JSValueRef Uint8ArrayWrite(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	TYPE toWrite = (TYPE)V2N(arguments[0]);
	p[(size_t)V2N(arguments[1])] = toWrite;

	return arguments[0];
}

JSValueRef Uint8ArrayToArray(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSValueRef jlen;
	JSValueRef *val;
	JSStringRef jls;
	TYPE *p;
	size_t count = 0;

	if (argumentCount > 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	p = (TYPE *)priv->data;
	count = priv->count;

	if (argumentCount == 0) {

		val = malloc(count * sizeof(JSValueRef));

		for (int i = 0; i < count; i++) {
			val[i] = N2V(p[i]);
		}

		ret = JSObjectMakeArray(ctx, count, val, exception);
	}
	else if (argumentCount == 1) {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t j = 0;
		if (offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(count * sizeof(JSValueRef));

		for (int i = offset; i < count; i++) {
			val[j] = N2V(p[i]);
			j++;
		}

		ret = JSObjectMakeArray(ctx, count - offset, val, exception);
	}
	else {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t toCopy = (size_t)V2N(arguments[1]);
		size_t j = 0;
		count -= offset;
		if (toCopy > count || offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(toCopy * sizeof(JSValueRef));

		for (int i = offset; ; i++) {
			val[j] = N2V(p[i]);
			if (j == toCopy)
				break;
			j++;
		}

		ret = JSObjectMakeArray(ctx, toCopy, val, exception);
	}

	free(val);
	return ret;
}

JSObjectRef Uint8ArrayNew(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSStringRef jls;
	JSObjectRef prop;
	size_t count = 0;
	void *p = NULL;

	if (argumentCount == 0 || argumentCount > 1)
		return JSValueToObject(ctx, N2V(0), exception);

	JSType type = JSValueGetType(ctx, arguments[0]);
	if (type == kJSTypeNumber) {
		count = (size_t)V2N(arguments[0]);
		p = calloc(count, sizeof(TYPE));
	}
	else if (type == kJSTypeObject) {
		JSObjectRef jarr;
		JSValueRef jlen;
		JSValueRef pres;
		TYPE *pval;

		jarr = JSValueToObject(ctx, arguments[0], exception);
		jls = JSStringCreateWithUTF8CString("length");
		jlen = JSObjectGetProperty(ctx, jarr, jls, exception);
		JSStringRelease(jls);
		count = (size_t)V2N(jlen);
		p = calloc(count, sizeof(TYPE));
		pval = (TYPE *)p;

		for (int i = 0; i < count; i++) {
			pres = JSObjectGetPropertyAtIndex(ctx, jarr, i, exception);
			pval[i] = (TYPE)V2N(pres);
		}
	}
	else {
		return JSValueToObject(ctx, N2V(0), exception);
	}

	LIExtTypedArrayPriv *priv = malloc(sizeof(LIExtTypedArrayPriv));
	priv->data = p;
	priv->size = count * sizeof(TYPE);
	priv->count = count;
	ret = JSObjectMake(ctx, sdef_Uint8Array, priv);

	jls = JSStringCreateWithUTF8CString("length");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->count), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	jls = JSStringCreateWithUTF8CString("byteLength");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->size), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	return ret;
}

JSValueRef Uint8ArrayGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] == 0x6C) { //length
		return N2V(priv->count);
	}
	else if (buf[0] == 0x62) { //byteLength
		return N2V(priv->size);
	}
	else if ((buf[0] >= 0x30 && buf[0] <= 0x39)) {
		TYPE *dptr = (TYPE *)priv->data;
		return N2V(dptr[calculateIndex(buf, len)]);
	}

	return NULL;
}

bool Uint8ArraySetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] < 0x30 || buf[0] > 0x39)
		return false;

	TYPE *dptr = (TYPE *)priv->data;
	dptr[calculateIndex(buf, len)] = (TYPE)V2N(value);

	return true;
}

static JSStaticFunction Uint8ArrayFuncs[] = {
	{ "read", Uint8ArrayRead, kJSPropertyAttributeNone },
	{ "write", Uint8ArrayWrite, kJSPropertyAttributeNone },
	{ "toArray", Uint8ArrayToArray, kJSPropertyAttributeNone },
	{ "copy", ArrayCopy, kJSPropertyAttributeNone },
	{ 0, 0, 0, 0 }
};

#undef TYPE

#define TYPE	int16_t

JSValueRef Int16ArrayRead(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 1)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	return N2V(p[(size_t)V2N(arguments[0])]);
}

JSValueRef Int16ArrayWrite(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	TYPE toWrite = (TYPE)V2N(arguments[0]);
	p[(size_t)V2N(arguments[1])] = toWrite;

	return arguments[0];
}

JSValueRef Int16ArrayToArray(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSValueRef jlen;
	JSValueRef *val;
	JSStringRef jls;
	TYPE *p;
	size_t count = 0;

	if (argumentCount > 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	p = (TYPE *)priv->data;
	count = priv->count;

	if (argumentCount == 0) {

		val = malloc(count * sizeof(JSValueRef));

		for (int i = 0; i < count; i++) {
			val[i] = N2V(p[i]);
		}

		ret = JSObjectMakeArray(ctx, count, val, exception);
	}
	else if (argumentCount == 1) {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t j = 0;
		if (offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(count * sizeof(JSValueRef));

		for (int i = offset; i < count; i++) {
			val[j] = N2V(p[i]);
			j++;
		}

		ret = JSObjectMakeArray(ctx, count - offset, val, exception);
	}
	else {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t toCopy = (size_t)V2N(arguments[1]);
		size_t j = 0;
		count -= offset;
		if (toCopy > count || offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(toCopy * sizeof(JSValueRef));

		for (int i = offset; ; i++) {
			val[j] = N2V(p[i]);
			if (j == toCopy)
				break;
			j++;
		}

		ret = JSObjectMakeArray(ctx, toCopy, val, exception);
	}

	free(val);
	return ret;
}

JSObjectRef Int16ArrayNew(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSStringRef jls;
	JSObjectRef prop;
	size_t count = 0;
	void *p = NULL;

	if (argumentCount == 0 || argumentCount > 1)
		return JSValueToObject(ctx, N2V(0), exception);

	JSType type = JSValueGetType(ctx, arguments[0]);
	if (type == kJSTypeNumber) {
		count = (size_t)V2N(arguments[0]);
		p = calloc(count, sizeof(TYPE));
	}
	else if (type == kJSTypeObject) {
		JSObjectRef jarr;
		JSValueRef jlen;
		JSValueRef pres;
		TYPE *pval;

		jarr = JSValueToObject(ctx, arguments[0], exception);
		jls = JSStringCreateWithUTF8CString("length");
		jlen = JSObjectGetProperty(ctx, jarr, jls, exception);
		JSStringRelease(jls);
		count = (size_t)V2N(jlen);
		p = calloc(count, sizeof(TYPE));
		pval = (TYPE *)p;

		for (int i = 0; i < count; i++) {
			pres = JSObjectGetPropertyAtIndex(ctx, jarr, i, exception);
			pval[i] = (TYPE)V2N(pres);
		}
	}
	else {
		return JSValueToObject(ctx, N2V(0), exception);
	}

	LIExtTypedArrayPriv *priv = malloc(sizeof(LIExtTypedArrayPriv));
	priv->data = p;
	priv->size = count * sizeof(TYPE);
	priv->count = count;
	ret = JSObjectMake(ctx, sdef_Int16Array, priv);

	jls = JSStringCreateWithUTF8CString("length");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->count), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	jls = JSStringCreateWithUTF8CString("byteLength");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->size), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	return ret;
}

JSValueRef Int16ArrayGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] == 0x6C) { //length
		return N2V(priv->count);
	}
	else if (buf[0] == 0x62) { //byteLength
		return N2V(priv->size);
	}
	else if ((buf[0] >= 0x30 && buf[0] <= 0x39)) {
		TYPE *dptr = (TYPE *)priv->data;
		return N2V(dptr[calculateIndex(buf, len)]);
	}

	return NULL;
}

bool Int16ArraySetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] < 0x30 || buf[0] > 0x39)
		return false;

	TYPE *dptr = (TYPE *)priv->data;
	dptr[calculateIndex(buf, len)] = (TYPE)V2N(value);

	return true;
}

static JSStaticFunction Int16ArrayFuncs[] = {
	{ "read", Int16ArrayRead, kJSPropertyAttributeNone },
	{ "write", Int16ArrayWrite, kJSPropertyAttributeNone },
	{ "toArray", Int16ArrayToArray, kJSPropertyAttributeNone },
	{ "copy", ArrayCopy, kJSPropertyAttributeNone },
	{ 0, 0, 0, 0 }
};

#undef TYPE

#define TYPE	uint16_t

JSValueRef Uint16ArrayRead(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 1)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	return N2V(p[(size_t)V2N(arguments[0])]);
}

JSValueRef Uint16ArrayWrite(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	TYPE toWrite = (TYPE)V2N(arguments[0]);
	p[(size_t)V2N(arguments[1])] = toWrite;

	return arguments[0];
}

JSValueRef Uint16ArrayToArray(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSValueRef jlen;
	JSValueRef *val;
	JSStringRef jls;
	TYPE *p;
	size_t count = 0;

	if (argumentCount > 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	p = (TYPE *)priv->data;
	count = priv->count;

	if (argumentCount == 0) {

		val = malloc(count * sizeof(JSValueRef));

		for (int i = 0; i < count; i++) {
			val[i] = N2V(p[i]);
		}

		ret = JSObjectMakeArray(ctx, count, val, exception);
	}
	else if (argumentCount == 1) {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t j = 0;
		if (offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(count * sizeof(JSValueRef));

		for (int i = offset; i < count; i++) {
			val[j] = N2V(p[i]);
			j++;
		}

		ret = JSObjectMakeArray(ctx, count - offset, val, exception);
	}
	else {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t toCopy = (size_t)V2N(arguments[1]);
		size_t j = 0;
		count -= offset;
		if (toCopy > count || offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(toCopy * sizeof(JSValueRef));

		for (int i = offset; ; i++) {
			val[j] = N2V(p[i]);
			if (j == toCopy)
				break;
			j++;
		}

		ret = JSObjectMakeArray(ctx, toCopy, val, exception);
	}

	free(val);
	return ret;
}

JSObjectRef Uint16ArrayNew(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSStringRef jls;
	JSObjectRef prop;
	size_t count = 0;
	void *p = NULL;

	if (argumentCount == 0 || argumentCount > 1)
		return JSValueToObject(ctx, N2V(0), exception);

	JSType type = JSValueGetType(ctx, arguments[0]);
	if (type == kJSTypeNumber) {
		count = (size_t)V2N(arguments[0]);
		p = calloc(count, sizeof(TYPE));
	}
	else if (type == kJSTypeObject) {
		JSObjectRef jarr;
		JSValueRef jlen;
		JSValueRef pres;
		TYPE *pval;

		jarr = JSValueToObject(ctx, arguments[0], exception);
		jls = JSStringCreateWithUTF8CString("length");
		jlen = JSObjectGetProperty(ctx, jarr, jls, exception);
		JSStringRelease(jls);
		count = (size_t)V2N(jlen);
		p = calloc(count, sizeof(TYPE));
		pval = (TYPE *)p;

		for (int i = 0; i < count; i++) {
			pres = JSObjectGetPropertyAtIndex(ctx, jarr, i, exception);
			pval[i] = (TYPE)V2N(pres);
		}
	}
	else {
		return JSValueToObject(ctx, N2V(0), exception);
	}

	LIExtTypedArrayPriv *priv = malloc(sizeof(LIExtTypedArrayPriv));
	priv->data = p;
	priv->size = count * sizeof(TYPE);
	priv->count = count;
	ret = JSObjectMake(ctx, sdef_Uint16Array, priv);

	jls = JSStringCreateWithUTF8CString("length");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->count), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	jls = JSStringCreateWithUTF8CString("byteLength");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->size), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	return ret;
}

JSValueRef Uint16ArrayGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] == 0x6C) { //length
		return N2V(priv->count);
	}
	else if (buf[0] == 0x62) { //byteLength
		return N2V(priv->size);
	}
	else if ((buf[0] >= 0x30 && buf[0] <= 0x39)) {
		TYPE *dptr = (TYPE *)priv->data;
		return N2V(dptr[calculateIndex(buf, len)]);
	}

	return NULL;
}

bool Uint16ArraySetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] < 0x30 || buf[0] > 0x39)
		return false;

	TYPE *dptr = (TYPE *)priv->data;
	dptr[calculateIndex(buf, len)] = (TYPE)V2N(value);

	return true;
}

static JSStaticFunction Uint16ArrayFuncs[] = {
	{ "read", Uint16ArrayRead, kJSPropertyAttributeNone },
	{ "write", Uint16ArrayWrite, kJSPropertyAttributeNone },
	{ "toArray", Uint16ArrayToArray, kJSPropertyAttributeNone },
	{ "copy", ArrayCopy, kJSPropertyAttributeNone },
	{ 0, 0, 0, 0 }
};

#undef TYPE

#define TYPE	int32_t

JSValueRef Int32ArrayRead(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 1)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	return N2V(p[(size_t)V2N(arguments[0])]);
}

JSValueRef Int32ArrayWrite(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	TYPE toWrite = (TYPE)V2N(arguments[0]);
	p[(size_t)V2N(arguments[1])] = toWrite;

	return arguments[0];
}

JSValueRef Int32ArrayToArray(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSValueRef jlen;
	JSValueRef *val;
	JSStringRef jls;
	TYPE *p;
	size_t count = 0;

	if (argumentCount > 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	p = (TYPE *)priv->data;
	count = priv->count;

	if (argumentCount == 0) {

		val = malloc(count * sizeof(JSValueRef));

		for (int i = 0; i < count; i++) {
			val[i] = N2V(p[i]);
		}

		ret = JSObjectMakeArray(ctx, count, val, exception);
	}
	else if (argumentCount == 1) {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t j = 0;
		if (offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(count * sizeof(JSValueRef));

		for (int i = offset; i < count; i++) {
			val[j] = N2V(p[i]);
			j++;
		}

		ret = JSObjectMakeArray(ctx, count - offset, val, exception);
	}
	else {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t toCopy = (size_t)V2N(arguments[1]);
		size_t j = 0;
		count -= offset;
		if (toCopy > count || offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(toCopy * sizeof(JSValueRef));

		for (int i = offset; ; i++) {
			val[j] = N2V(p[i]);
			if (j == toCopy)
				break;
			j++;
		}

		ret = JSObjectMakeArray(ctx, toCopy, val, exception);
	}

	free(val);
	return ret;
}

JSObjectRef Int32ArrayNew(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSStringRef jls;
	JSObjectRef prop;
	size_t count = 0;
	void *p = NULL;

	if (argumentCount == 0 || argumentCount > 1)
		return JSValueToObject(ctx, N2V(0), exception);

	JSType type = JSValueGetType(ctx, arguments[0]);
	if (type == kJSTypeNumber) {
		count = (size_t)V2N(arguments[0]);
		p = calloc(count, sizeof(TYPE));
	}
	else if (type == kJSTypeObject) {
		JSObjectRef jarr;
		JSValueRef jlen;
		JSValueRef pres;
		TYPE *pval;

		jarr = JSValueToObject(ctx, arguments[0], exception);
		jls = JSStringCreateWithUTF8CString("length");
		jlen = JSObjectGetProperty(ctx, jarr, jls, exception);
		JSStringRelease(jls);
		count = (size_t)V2N(jlen);
		p = calloc(count, sizeof(TYPE));
		pval = (TYPE *)p;

		for (int i = 0; i < count; i++) {
			pres = JSObjectGetPropertyAtIndex(ctx, jarr, i, exception);
			pval[i] = (TYPE)V2N(pres);
		}
	}
	else {
		return JSValueToObject(ctx, N2V(0), exception);
	}

	LIExtTypedArrayPriv *priv = malloc(sizeof(LIExtTypedArrayPriv));
	priv->data = p;
	priv->size = count * sizeof(TYPE);
	priv->count = count;
	ret = JSObjectMake(ctx, sdef_Int32Array, priv);

	jls = JSStringCreateWithUTF8CString("length");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->count), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	jls = JSStringCreateWithUTF8CString("byteLength");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->size), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	return ret;
}

JSValueRef Int32ArrayGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] == 0x6C) { //length
		return N2V(priv->count);
	}
	else if (buf[0] == 0x62) { //byteLength
		return N2V(priv->size);
	}
	else if ((buf[0] >= 0x30 && buf[0] <= 0x39)) {
		TYPE *dptr = (TYPE *)priv->data;
		return N2V(dptr[calculateIndex(buf, len)]);
	}

	return NULL;
}

bool Int32ArraySetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] < 0x30 || buf[0] > 0x39)
		return false;

	TYPE *dptr = (TYPE *)priv->data;
	dptr[calculateIndex(buf, len)] = (TYPE)V2N(value);

	return true;
}

static JSStaticFunction Int32ArrayFuncs[] = {
	{ "read", Int32ArrayRead, kJSPropertyAttributeNone },
	{ "write", Int32ArrayWrite, kJSPropertyAttributeNone },
	{ "toArray", Int32ArrayToArray, kJSPropertyAttributeNone },
	{ "copy", ArrayCopy, kJSPropertyAttributeNone },
	{ 0, 0, 0, 0 }
};

#undef TYPE

#define TYPE	uint32_t

JSValueRef Uint32ArrayRead(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 1)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	return N2V(p[(size_t)V2N(arguments[0])]);
}

JSValueRef Uint32ArrayWrite(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	TYPE toWrite = (TYPE)V2N(arguments[0]);
	p[(size_t)V2N(arguments[1])] = toWrite;

	return arguments[0];
}

JSValueRef Uint32ArrayToArray(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSValueRef jlen;
	JSValueRef *val;
	JSStringRef jls;
	TYPE *p;
	size_t count = 0;

	if (argumentCount > 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	p = (TYPE *)priv->data;
	count = priv->count;

	if (argumentCount == 0) {

		val = malloc(count * sizeof(JSValueRef));

		for (int i = 0; i < count; i++) {
			val[i] = N2V(p[i]);
		}

		ret = JSObjectMakeArray(ctx, count, val, exception);
	}
	else if (argumentCount == 1) {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t j = 0;
		if (offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(count * sizeof(JSValueRef));

		for (int i = offset; i < count; i++) {
			val[j] = N2V(p[i]);
			j++;
		}

		ret = JSObjectMakeArray(ctx, count - offset, val, exception);
	}
	else {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t toCopy = (size_t)V2N(arguments[1]);
		size_t j = 0;
		count -= offset;
		if (toCopy > count || offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(toCopy * sizeof(JSValueRef));

		for (int i = offset; ; i++) {
			val[j] = N2V(p[i]);
			if (j == toCopy)
				break;
			j++;
		}

		ret = JSObjectMakeArray(ctx, toCopy, val, exception);
	}

	free(val);
	return ret;
}

JSObjectRef Uint32ArrayNew(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSStringRef jls;
	JSObjectRef prop;
	size_t count = 0;
	void *p = NULL;

	if (argumentCount == 0 || argumentCount > 1)
		return JSValueToObject(ctx, N2V(0), exception);

	JSType type = JSValueGetType(ctx, arguments[0]);
	if (type == kJSTypeNumber) {
		count = (size_t)V2N(arguments[0]);
		p = calloc(count, sizeof(TYPE));
	}
	else if (type == kJSTypeObject) {
		JSObjectRef jarr;
		JSValueRef jlen;
		JSValueRef pres;
		TYPE *pval;

		jarr = JSValueToObject(ctx, arguments[0], exception);
		jls = JSStringCreateWithUTF8CString("length");
		jlen = JSObjectGetProperty(ctx, jarr, jls, exception);
		JSStringRelease(jls);
		count = (size_t)V2N(jlen);
		p = calloc(count, sizeof(TYPE));
		pval = (TYPE *)p;

		for (int i = 0; i < count; i++) {
			pres = JSObjectGetPropertyAtIndex(ctx, jarr, i, exception);
			pval[i] = (TYPE)V2N(pres);
		}
	}
	else {
		return JSValueToObject(ctx, N2V(0), exception);
	}

	LIExtTypedArrayPriv *priv = malloc(sizeof(LIExtTypedArrayPriv));
	priv->data = p;
	priv->size = count * sizeof(TYPE);
	priv->count = count;
	ret = JSObjectMake(ctx, sdef_Uint32Array, priv);

	jls = JSStringCreateWithUTF8CString("length");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->count), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	jls = JSStringCreateWithUTF8CString("byteLength");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->size), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	return ret;
}

JSValueRef Uint32ArrayGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] == 0x6C) { //length
		return N2V(priv->count);
	}
	else if (buf[0] == 0x62) { //byteLength
		return N2V(priv->size);
	}
	else if ((buf[0] >= 0x30 && buf[0] <= 0x39)) {
		TYPE *dptr = (TYPE *)priv->data;
		return N2V(dptr[calculateIndex(buf, len)]);
	}

	return NULL;
}

bool Uint32ArraySetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] < 0x30 || buf[0] > 0x39)
		return false;

	TYPE *dptr = (TYPE *)priv->data;
	dptr[calculateIndex(buf, len)] = (TYPE)V2N(value);

	return true;
}

static JSStaticFunction Uint32ArrayFuncs[] = {
	{ "read", Uint32ArrayRead, kJSPropertyAttributeNone },
	{ "write", Uint32ArrayWrite, kJSPropertyAttributeNone },
	{ "toArray", Uint32ArrayToArray, kJSPropertyAttributeNone },
	{ "copy", ArrayCopy, kJSPropertyAttributeNone },
	{ 0, 0, 0, 0 }
};

#undef TYPE

#undef N2V
#define N2V(x) JSValueMakeNumber(ctx, (double)(x))
#define TYPE	float

JSValueRef Float32ArrayRead(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 1)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	return N2V(p[(size_t)V2N(arguments[0])]);
}

JSValueRef Float32ArrayWrite(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	if (argumentCount != 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	TYPE *p = (TYPE *)priv->data;
	TYPE toWrite = (TYPE)V2N(arguments[0]);
	p[(size_t)V2N(arguments[1])] = toWrite;

	return arguments[0];
}

JSValueRef Float32ArrayToArray(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSValueRef jlen;
	JSValueRef *val;
	JSStringRef jls;
	TYPE *p;
	size_t count = 0;

	if (argumentCount > 2)
		return JSValueMakeUndefined(ctx);

	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(thisObject);
	p = (TYPE *)priv->data;
	count = priv->count;

	if (argumentCount == 0) {

		val = malloc(count * sizeof(JSValueRef));

		for (int i = 0; i < count; i++) {
			val[i] = N2V(p[i]);
		}

		ret = JSObjectMakeArray(ctx, count, val, exception);
	}
	else if (argumentCount == 1) {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t j = 0;
		if (offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(count * sizeof(JSValueRef));

		for (int i = offset; i < count; i++) {
			val[j] = N2V(p[i]);
			j++;
		}

		ret = JSObjectMakeArray(ctx, count - offset, val, exception);
	}
	else {

		size_t offset = (size_t)V2N(arguments[0]);
		size_t toCopy = (size_t)V2N(arguments[1]);
		size_t j = 0;
		count -= offset;
		if (toCopy > count || offset > count)
			return JSValueMakeUndefined(ctx);
		val = malloc(toCopy * sizeof(JSValueRef));

		for (int i = offset; ; i++) {
			val[j] = N2V(p[i]);
			if (j == toCopy)
				break;
			j++;
		}

		ret = JSObjectMakeArray(ctx, toCopy, val, exception);
	}

	free(val);
	return ret;
}

JSObjectRef Float32ArrayNew(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
	JSObjectRef ret;
	JSStringRef jls;
	JSObjectRef prop;
	size_t count = 0;
	void *p = NULL;

	if (argumentCount == 0 || argumentCount > 1)
		return JSValueToObject(ctx, N2V(0), exception);

	JSType type = JSValueGetType(ctx, arguments[0]);
	if (type == kJSTypeNumber) {
		count = (size_t)V2N(arguments[0]);
		p = calloc(count, sizeof(TYPE));
	}
	else if (type == kJSTypeObject) {
		JSObjectRef jarr;
		JSValueRef jlen;
		JSValueRef pres;
		TYPE *pval;

		jarr = JSValueToObject(ctx, arguments[0], exception);
		jls = JSStringCreateWithUTF8CString("length");
		jlen = JSObjectGetProperty(ctx, jarr, jls, exception);
		JSStringRelease(jls);
		count = (size_t)V2N(jlen);
		p = calloc(count, sizeof(TYPE));
		pval = (TYPE *)p;

		for (int i = 0; i < count; i++) {
			pres = JSObjectGetPropertyAtIndex(ctx, jarr, i, exception);
			pval[i] = (TYPE)V2N(pres);
		}
	}
	else {
		return JSValueToObject(ctx, N2V(0), exception);
	}

	LIExtTypedArrayPriv *priv = malloc(sizeof(LIExtTypedArrayPriv));
	priv->data = p;
	priv->size = count * sizeof(TYPE);
	priv->count = count;
	ret = JSObjectMake(ctx, sdef_Float32Array, priv);

	jls = JSStringCreateWithUTF8CString("length");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->count), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	jls = JSStringCreateWithUTF8CString("byteLength");
	JSObjectSetProperty(ctx, ret, jls, N2V(priv->size), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(jls);

	return ret;
}

JSValueRef Float32ArrayGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] == 0x6C) { //length
		return N2V(priv->count);
	}
	else if (buf[0] == 0x62) { //byteLength
		return N2V(priv->size);
	}
	else if ((buf[0] >= 0x30 && buf[0] <= 0x39)) {
		TYPE *dptr = (TYPE *)priv->data;
		return N2V(dptr[calculateIndex(buf, len)]);
	}

	return NULL;
}

bool Float32ArraySetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
	uint16_t *buf = (uint16_t *)JSStringGetCharactersPtr(propertyName);
	size_t len = JSStringGetLength(propertyName);
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(object);

	if (buf[0] < 0x30 || buf[0] > 0x39)
		return false;

	TYPE *dptr = (TYPE *)priv->data;
	dptr[calculateIndex(buf, len)] = (TYPE)V2N(value);

	return true;
}

static JSStaticFunction Float32ArrayFuncs[] = {
	{ "read", Float32ArrayRead, kJSPropertyAttributeNone },
	{ "write", Float32ArrayWrite, kJSPropertyAttributeNone },
	{ "toArray", Float32ArrayToArray, kJSPropertyAttributeNone },
	{ "copy", ArrayCopy, kJSPropertyAttributeNone },
	{ 0, 0, 0, 0 }
};

#undef TYPE

void liextCreateClassTypedArray(JSContextRef ctx, JSValueRef *excp)
{
	JSObjectRef classObj;
	JSObjectRef globalObj = JSContextGetGlobalObject(ctx);
	JSClassDefinition classDesc;
	JSStringRef str;

	memset(&classDesc, 0, sizeof(JSClassDefinition));
	classDesc.className = "Int8Array";
	classDesc.callAsConstructor = Int8ArrayNew;
	classDesc.staticFunctions = Int8ArrayFuncs;
	classDesc.finalize = ArrayDelete;
	classDesc.getProperty = Int8ArrayGetProperty;
	classDesc.setProperty = Int8ArraySetProperty;

	sdef_Int8Array = JSClassCreate(&classDesc);
	classObj = JSObjectMake(ctx, sdef_Int8Array, NULL);
	str = JSStringCreateWithUTF8CString(classDesc.className);
	JSObjectSetProperty(ctx, globalObj, str, classObj, kJSPropertyAttributeNone, NULL);
	JSStringRelease(str);

	classDesc.className = "Uint8Array";
	classDesc.callAsConstructor = Uint8ArrayNew;
	classDesc.staticFunctions = Uint8ArrayFuncs;
	classDesc.finalize = ArrayDelete;
	classDesc.getProperty = Uint8ArrayGetProperty;
	classDesc.setProperty = Uint8ArraySetProperty;

	sdef_Uint8Array = JSClassCreate(&classDesc);
	classObj = JSObjectMake(ctx, sdef_Uint8Array, NULL);
	str = JSStringCreateWithUTF8CString(classDesc.className);
	JSObjectSetProperty(ctx, globalObj, str, classObj, kJSPropertyAttributeNone, NULL);
	JSStringRelease(str);

	classDesc.className = "Int16Array";
	classDesc.callAsConstructor = Int16ArrayNew;
	classDesc.staticFunctions = Int16ArrayFuncs;
	classDesc.finalize = ArrayDelete;
	classDesc.getProperty = Int16ArrayGetProperty;
	classDesc.setProperty = Int16ArraySetProperty;

	sdef_Int16Array = JSClassCreate(&classDesc);
	classObj = JSObjectMake(ctx, sdef_Int16Array, NULL);
	str = JSStringCreateWithUTF8CString(classDesc.className);
	JSObjectSetProperty(ctx, globalObj, str, classObj, kJSPropertyAttributeNone, NULL);
	JSStringRelease(str);

	classDesc.className = "Uint16Array";
	classDesc.callAsConstructor = Uint16ArrayNew;
	classDesc.staticFunctions = Uint16ArrayFuncs;
	classDesc.finalize = ArrayDelete;
	classDesc.getProperty = Uint16ArrayGetProperty;
	classDesc.setProperty = Uint16ArraySetProperty;

	sdef_Uint16Array = JSClassCreate(&classDesc);
	classObj = JSObjectMake(ctx, sdef_Uint16Array, NULL);
	str = JSStringCreateWithUTF8CString(classDesc.className);
	JSObjectSetProperty(ctx, globalObj, str, classObj, kJSPropertyAttributeNone, NULL);
	JSStringRelease(str);

	classDesc.className = "Int32Array";
	classDesc.callAsConstructor = Int32ArrayNew;
	classDesc.staticFunctions = Int32ArrayFuncs;
	classDesc.finalize = ArrayDelete;
	classDesc.getProperty = Int32ArrayGetProperty;
	classDesc.setProperty = Int32ArraySetProperty;

	sdef_Int32Array = JSClassCreate(&classDesc);
	classObj = JSObjectMake(ctx, sdef_Int32Array, NULL);
	str = JSStringCreateWithUTF8CString(classDesc.className);
	JSObjectSetProperty(ctx, globalObj, str, classObj, kJSPropertyAttributeNone, NULL);
	JSStringRelease(str);

	classDesc.className = "Uint32Array";
	classDesc.callAsConstructor = Uint32ArrayNew;
	classDesc.staticFunctions = Uint32ArrayFuncs;
	classDesc.finalize = ArrayDelete;
	classDesc.getProperty = Uint32ArrayGetProperty;
	classDesc.setProperty = Uint32ArraySetProperty;

	sdef_Uint32Array = JSClassCreate(&classDesc);
	classObj = JSObjectMake(ctx, sdef_Uint32Array, NULL);
	str = JSStringCreateWithUTF8CString(classDesc.className);
	JSObjectSetProperty(ctx, globalObj, str, classObj, kJSPropertyAttributeNone, NULL);
	JSStringRelease(str);

	classDesc.className = "Float32Array";
	classDesc.callAsConstructor = Float32ArrayNew;
	classDesc.staticFunctions = Float32ArrayFuncs;
	classDesc.finalize = ArrayDelete;
	classDesc.getProperty = Float32ArrayGetProperty;
	classDesc.setProperty = Float32ArraySetProperty;

	sdef_Float32Array = JSClassCreate(&classDesc);
	classObj = JSObjectMake(ctx, sdef_Float32Array, NULL);
	str = JSStringCreateWithUTF8CString(classDesc.className);
	JSObjectSetProperty(ctx, globalObj, str, classObj, kJSPropertyAttributeNone, NULL);
	JSStringRelease(str);
}

void liextReleaseClassTypedArray(JSContextRef ctx, JSValueRef *excp)
{
	JSClassRelease(sdef_Int8Array);
	JSClassRelease(sdef_Uint8Array);
	JSClassRelease(sdef_Int16Array);
	JSClassRelease(sdef_Uint16Array);
	JSClassRelease(sdef_Int32Array);
	JSClassRelease(sdef_Uint32Array);
	JSClassRelease(sdef_Float32Array);
}