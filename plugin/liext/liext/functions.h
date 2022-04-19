#pragma once

#include <JavaScriptCore.h>

void liextAddFunctionProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp);
void liextRemoveFunctionProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp);
