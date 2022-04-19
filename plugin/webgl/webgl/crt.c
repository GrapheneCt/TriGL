#include <kernel.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <trilithium/plugin.h>
#include <JavaScriptCore.h>

#include "functions.h"
#include "constants.h"

#define STRING(x) #x

static JSClassRef s_webglClass;
static LIPlatformFuncs s_platform;

void webglClassFinalize(JSObjectRef object)
{
	void *privData = JSObjectGetPrivate(object);
	if (privData)
		s_platform.memfree(privData);
}

int webglCreateClass()
{
	JSClassDefinition def;

	memset(&def, 0, sizeof(JSClassDefinition));
	def.className = "Cgl";
	def.finalize = webglClassFinalize;

	s_webglClass = JSClassCreate(&def);

	return 0;
}

int webglReleaseClass()
{
	JSClassRelease(s_webglClass);

	return 0;
}

int webglAddPluginProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp)
{
	webglAddConstantProperties(ctx, obj, excp);
	webglAddFunctionProperties(ctx, obj, excp);

	return 0;
}

int webglRemovePluginProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp)
{
	webglRemoveConstantProperties(ctx, obj, excp);
	webglRemoveFunctionProperties(ctx, obj, excp);

	return 0;
}

int webglInit(LIPlatformFuncs *platform, LIPluginFuncs *plugin)
{
	memcpy(&s_platform, platform, platform->size);

	plugin->createClass = webglCreateClass;
	plugin->releaseClass = webglReleaseClass;
	plugin->addPluginProperties = webglAddPluginProperties;
	plugin->removePluginProperties = webglRemovePluginProperties;

	return 0;
}

int webglExit()
{
	return 0;
}

int module_start(SceSize args, const void * argp)
{
	SceInt32 ret = SCE_KERNEL_START_SUCCESS;
	SceInt32 *argarr = (SceInt32 *)argp;

	void(*setModule)(LiPluginModule *cb, int a2);

	if (sceKernelLoadStartModule("app0:module/libgpu_es4_ext.suprx", 0, NULL, 0, NULL, NULL) <= 0 ||
		sceKernelLoadStartModule("app0:module/libIMGEGL.suprx", 0, NULL, 0, NULL, NULL) <= 0) {
		ret = SCE_KERNEL_START_FAILED;
	}

	if (args == 0 || argp == NULL || ret == SCE_KERNEL_START_FAILED) {
		ret = SCE_KERNEL_START_FAILED;
	}
	else {
		LiPluginModule module;
		memset(&module, 0, sizeof(LiPluginModule));
		module.initFunc = webglInit;
		module.exitFunc = webglExit;
		setModule = (void *)(argarr[0]);
		setModule(&module, argarr[1]);
	}

	return ret;
}
