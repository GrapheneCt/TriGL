#include <kernel.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <trilithium/plugin.h>
#include <JavaScriptCore.h>

#include "functions.h"
#include "constants.h"

static JSClassRef s_liextClass;
static LIPlatformFuncs s_platform;

void liextClassFinalize(JSObjectRef object)
{
	void *privData = JSObjectGetPrivate(object);
	if (privData)
		s_platform.memfree(privData);
}

int liextCreateClass()
{
	JSClassDefinition def;

	memset(&def, 0, sizeof(JSClassDefinition));
	def.className = "Cliext";
	def.finalize = liextClassFinalize;
	//def.staticValues = liextGetStaticArrayPointer();

	s_liextClass = JSClassCreate(&def);

	return 0;
}

int liextReleaseClass()
{
	JSClassRelease(s_liextClass);

	return 0;
}

int liextAddPluginProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp)
{
	liextAddConstantProperties(ctx, obj, excp);
	liextAddFunctionProperties(ctx, obj, excp);

	liextCreateClassTypedArray(ctx, excp);

	return 0;
}

int liextRemovePluginProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp)
{
	liextRemoveConstantProperties(ctx, obj, excp);
	liextRemoveFunctionProperties(ctx, obj, excp);

	liextReleaseClassTypedArray(ctx, excp);

	return 0;
}

int liextInit(LIPlatformFuncs *platform, LIPluginFuncs *plugin)
{
	memcpy(&s_platform, platform, platform->size);

	plugin->createClass = liextCreateClass;
	plugin->releaseClass = liextReleaseClass;
	plugin->addPluginProperties = liextAddPluginProperties;
	plugin->removePluginProperties = liextRemovePluginProperties;

	return 0;
}

int liextExit()
{
	return 0;
}

int module_start(SceSize args, const void * argp)
{
	SceInt32 ret = SCE_KERNEL_START_SUCCESS;
	SceInt32 *argarr = (SceInt32 *)argp;

	void(*setModule)(LiPluginModule *cb, int a2);

	if (args == 0 || argp == NULL) {
		ret = SCE_KERNEL_START_FAILED;
	}
	else {
		LiPluginModule module;
		memset(&module, 0, sizeof(LiPluginModule));
		module.initFunc = liextInit;
		module.exitFunc = liextExit;
		setModule = (void *)(argarr[0]);
		setModule(&module, argarr[1]);
	}

	return ret;
}
