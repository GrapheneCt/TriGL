#pragma once

#include <JavaScriptCore.h>

void webglAddConstantProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp);
void webglRemoveConstantProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp);
