/*
	Vita Development Suite Libraries
*/

#ifndef _PSP2_TRILITHIUM_PLUGIN_H_
#define _PSP2_TRILITHIUM_PLUGIN_H_

#include <JavaScriptCore.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int(*LiPluginModuleExit)(void);

typedef int(*LiPluginModuleUpdate)(void);

typedef int(*LiPluginModuleDraw)(void);

typedef struct LIPConfigDefinition {
	char *key;
	char *defaultValue;
	int(*validateFunc)(char *, char *, int, void *, void *);
	void *pKeyContext;
} LIPConfigDefinition;

typedef struct LIPlatformFuncs {
	uint16_t size;
	uint16_t version;
	void*(*memalloc)(size_t);
	void(*memfree)(void *);
	void*(*openConfig)(char *, struct LIPConfigDefinition *, void *, void(*)(void *));
	void*(*createConfig)(JSObjectRef, char *, struct LIPConfigDefinition *, void *, void(*)(void *));
	void(*closeConfig)(void *);
	char*(*getConfigString)(void *, char *);
	int(*setConfigString)(void *, char *, char *);
	uint8_t*(*getConfigData)(void *, char *, size_t *);
	int(*setConfigData)(void *, char *, uint8_t *, size_t);
	JSObjectRef(*getJSConfig)(void *);
	JSGlobalContextRef(*getJSContext)(void);
	JSValueRef(*makeException)(char *);
	void(*handleException)(JSValueRef);
} LIPlatformFuncs;

typedef int(*LP_JSPAddPluginPropertiesProcPtr)(JSContextRef, JSObjectRef, JSValueRef *);

typedef int(*LP_JSPRemovePluginPropertiesProcPtr)(JSContextRef, JSObjectRef, JSValueRef *);

typedef int(*LP_JSCreateClassProcPtr)(void);

typedef int(*LP_JSReleaseClassProcPtr)(void);

typedef struct LIPluginFuncs {
	uint16_t size;
	uint16_t version;
	LP_JSPAddPluginPropertiesProcPtr addPluginProperties;
	LP_JSPRemovePluginPropertiesProcPtr removePluginProperties;
	LP_JSCreateClassProcPtr createClass;
	LP_JSReleaseClassProcPtr releaseClass;
	int(*createObject)(JSObjectRef, JSContextRef);
	void *customInterface;
} LIPluginFuncs;

typedef struct LiPluginModule {
	char *type;
	int(*initFunc)(LIPlatformFuncs *, LIPluginFuncs *);
	LiPluginModuleExit exitFunc;
	LiPluginModuleUpdate updateFunc;
	LiPluginModuleDraw drawFunc;
} LiPluginModule;

#ifdef __cplusplus
}
#endif

#endif /* _PSP2_TRILITHIUM_PLUGIN_H_ */
