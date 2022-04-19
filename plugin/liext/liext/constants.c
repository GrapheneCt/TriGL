#include <kernel.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <JavaScriptCore.h>

#define SET_CONSTANT_PROPERTY(str) \
name = JSStringCreateWithUTF8CString(#str); \
JSObjectSetProperty(ctx, obj, name, JSValueMakeNumber(ctx, (double)SCE_##str), kJSPropertyAttributeReadOnly, excp); \
JSStringRelease(name);

#define DELETE_CONSTANT_PROPERTY(str) \
name = JSStringCreateWithUTF8CString(#str); \
JSObjectDeleteProperty(ctx, obj, name, excp); \
JSStringRelease(name);

void liextAddConstantProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp)
{
	JSStringRef name;

	SET_CONSTANT_PROPERTY(O_RDONLY)
	SET_CONSTANT_PROPERTY(O_WRONLY)
	SET_CONSTANT_PROPERTY(O_CREAT)
	SET_CONSTANT_PROPERTY(O_TRUNC)
	SET_CONSTANT_PROPERTY(O_APPEND)
	SET_CONSTANT_PROPERTY(SEEK_CUR)
	SET_CONSTANT_PROPERTY(SEEK_END)
	SET_CONSTANT_PROPERTY(SEEK_SET)
	SET_CONSTANT_PROPERTY(KERNEL_MODEL_VITATV)
	SET_CONSTANT_PROPERTY(KERNEL_MODEL_VITA)
}

void liextRemoveConstantProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp)
{
	JSStringRef name;

	DELETE_CONSTANT_PROPERTY(O_RDONLY)
	DELETE_CONSTANT_PROPERTY(O_WRONLY)
	DELETE_CONSTANT_PROPERTY(O_CREAT)
	DELETE_CONSTANT_PROPERTY(O_TRUNC)
	DELETE_CONSTANT_PROPERTY(O_APPEND)
	DELETE_CONSTANT_PROPERTY(SEEK_CUR)
	DELETE_CONSTANT_PROPERTY(SEEK_END)
	DELETE_CONSTANT_PROPERTY(SEEK_SET)
	DELETE_CONSTANT_PROPERTY(KERNEL_MODEL_VITATV)
	DELETE_CONSTANT_PROPERTY(KERNEL_MODEL_VITA)
}