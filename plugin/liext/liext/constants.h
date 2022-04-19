#pragma once

#include <JavaScriptCore.h>

void liextAddConstantProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp);
void liextRemoveConstantProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp);
