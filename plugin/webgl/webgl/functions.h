#pragma once

#include <JavaScriptCore.h>

void webglAddFunctionProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp);
void webglRemoveFunctionProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp);
