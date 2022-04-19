#include <kernel.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <gpu_es4/psp2_pvr_hint.h>

#include <JavaScriptCore.h>

#define PRINT_ARRAY_WARNING \
printf("[webgl] Potential usage of JS Array has been detected in %s, use TypedArray instead\n", __FUNCTION__);

#define SET_FUNCTION_PROPERTY(str) \
name = JSStringCreateWithUTF8CString(#str); \
prop = JSObjectMakeFunctionWithCallback(ctx, NULL, webgl_##str); \
JSObjectSetProperty(ctx, obj, name, prop, 0, excp); \
JSStringRelease(name);

#define DELETE_FUNCTION_PROPERTY(str) \
name = JSStringCreateWithUTF8CString(#str); \
JSObjectDeleteProperty(ctx, obj, name, excp); \
JSStringRelease(name);

#define V2N(x) JSValueToNumber(ctx, x, exception)
#define N2V(x) JSValueMakeNumber(ctx, (double)((int64_t)x))

#define WEBGL_FUNCTION_HEAD(str) JSValueRef webgl_##str(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)

#define MAX_ATTACHED_SHADERS		100
#define MAX_INFO_NAMELEN			256
#define MAX_PROGRAM_INFO_LOG_LEN	256
#define MAX_SHADER_INFO_LOG_LEN		256

typedef struct LIExtTypedArrayPriv {
	void *data;
	size_t size;
	size_t count;
} LIExtTypedArrayPriv;

static unsigned int s_width = 0;
static unsigned int s_height = 0;
static EGLDisplay dpy = NULL;
static EGLSurface surface = NULL;

WEBGL_FUNCTION_HEAD(activeTexture)
{
	glActiveTexture((GLenum)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(attachShader)
{
	glAttachShader((GLuint)V2N(arguments[0]), (GLuint)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(bindAttribLocation)
{
	char *buffer = NULL;
	size_t sz = 0;

	JSStringRef str = JSValueToStringCopy(ctx, arguments[2], exception);
	sz = JSStringGetMaximumUTF8CStringSize(str);
	buffer = malloc(sz);
	JSStringGetUTF8CString(str, buffer, sz);
	JSStringRelease(str);

	glBindAttribLocation((GLuint)V2N(arguments[0]), (GLuint)V2N(arguments[1]), buffer);
	free(buffer);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(bindBuffer)
{
	glBindBuffer((GLenum)V2N(arguments[0]), (GLuint)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(bindFramebuffer)
{
	glBindFramebuffer((GLenum)V2N(arguments[0]), (GLuint)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(bindRenderbuffer)
{
	glBindRenderbuffer((GLenum)V2N(arguments[0]), (GLuint)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(bindTexture)
{
	glBindTexture((GLenum)V2N(arguments[0]), (GLuint)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(blendColor)
{
	glBlendColor((GLclampf)V2N(arguments[0]), (GLclampf)V2N(arguments[1]), (GLclampf)V2N(arguments[2]), (GLclampf)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(blendEquation)
{
	glBlendEquation((GLenum)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(blendEquationSeparate)
{
	glBlendEquationSeparate((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(blendFunc)
{
	glBlendFunc((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(blendFuncSeparate)
{
	glBlendFuncSeparate((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), (GLenum)V2N(arguments[2]), (GLenum)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(bufferData)
{
	if (!JSValueIsObject(ctx, arguments[1])) {
		glBufferData((GLenum)V2N(arguments[0]), (GLsizeiptr)V2N(arguments[1]), NULL, (GLenum)V2N(arguments[2]));
	}
	else {
		LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[1], exception));
		if (priv == NULL) {
			PRINT_ARRAY_WARNING
			abort();
		}

		glBufferData((GLenum)V2N(arguments[0]), (GLsizeiptr)priv->size, priv->data, (GLenum)V2N(arguments[2]));
	}

	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(bufferSubData)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[2], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glBufferSubData((GLenum)V2N(arguments[0]), (GLintptr)V2N(arguments[1]), priv->size, priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(checkFramebufferStatus)
{
	return N2V(glCheckFramebufferStatus((GLenum)V2N(arguments[0])));
}

WEBGL_FUNCTION_HEAD(clear)
{
	glClear((GLbitfield)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(clearColor)
{
	glClearColor((GLclampf)V2N(arguments[0]), (GLclampf)V2N(arguments[1]), (GLclampf)V2N(arguments[2]), (GLclampf)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(clearDepth)
{
	glClearDepthf((GLclampf)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(clearStencil)
{
	glClearStencil((GLint)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(colorMask)
{
	glColorMask((GLboolean)V2N(arguments[0]), (GLboolean)V2N(arguments[1]), (GLboolean)V2N(arguments[2]), (GLboolean)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(compileShader)
{
	glCompileShader((GLuint)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(compressedTexImage2D)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[6], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glCompressedTexImage2D((GLenum)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLenum)V2N(arguments[2]), (GLsizei)V2N(arguments[3]), (GLsizei)V2N(arguments[4]), (GLint)V2N(arguments[5]), priv->size, priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(compressedTexSubImage2D)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[7], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glCompressedTexSubImage2D((GLenum)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLint)V2N(arguments[2]), (GLint)V2N(arguments[3]), (GLsizei)V2N(arguments[4]), (GLsizei)V2N(arguments[5]), (GLenum)V2N(arguments[6]), priv->size, priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(copyTexImage2D)
{
	glCopyTexImage2D((GLenum)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLenum)V2N(arguments[2]), (GLint)V2N(arguments[3]), (GLint)V2N(arguments[4]), (GLsizei)V2N(arguments[5]), (GLsizei)V2N(arguments[6]), (GLint)V2N(arguments[7]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(copyTexSubImage2D)
{
	glCopyTexSubImage2D((GLenum)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLint)V2N(arguments[2]), (GLint)V2N(arguments[3]), (GLint)V2N(arguments[4]), (GLint)V2N(arguments[5]), (GLsizei)V2N(arguments[6]), (GLsizei)V2N(arguments[7]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(createProgram)
{
	return N2V(glCreateProgram());
}

WEBGL_FUNCTION_HEAD(createShader)
{
	return N2V(glCreateShader((GLenum)V2N(arguments[0])));
}

WEBGL_FUNCTION_HEAD(cullFace)
{
	glCullFace((GLenum)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(deleteBuffer)
{
	GLuint buf = (GLuint)V2N(arguments[0]);
	glDeleteBuffers(1, &buf);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(deleteFramebuffer)
{
	GLuint buf = (GLuint)V2N(arguments[0]);
	glDeleteFramebuffers(1, &buf);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(deleteProgram)
{
	glDeleteProgram((GLuint)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(deleteRenderbuffer)
{
	GLuint buf = (GLuint)V2N(arguments[0]);
	glDeleteRenderbuffers(1, &buf);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(deleteShader)
{
	glDeleteShader((GLuint)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(deleteTexture)
{
	GLuint buf = (GLuint)V2N(arguments[0]);
	glDeleteTextures(1, &buf);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(depthFunc)
{
	glDepthFunc((GLenum)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(depthMask)
{
	glDepthMask((GLboolean)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(depthRange)
{
	glDepthRangef((GLclampf)V2N(arguments[0]), (GLclampf)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(detachShader)
{
	glDetachShader((GLuint)V2N(arguments[0]), (GLuint)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(disable)
{
	glDisable((GLenum)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(disableVertexAttribArray)
{
	glDisableVertexAttribArray((GLuint)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(drawArrays)
{
	glDrawArrays((GLenum)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLsizei)V2N(arguments[2]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(drawElements)
{
	glDrawElements((GLenum)V2N(arguments[0]), (GLsizei)V2N(arguments[1]), (GLenum)V2N(arguments[2]), (GLvoid *)((int64_t)V2N(arguments[3])));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(enable)
{
	glEnable((GLenum)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(enableVertexAttribArray)
{
	glEnableVertexAttribArray((GLuint)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(finish)
{
	glFinish();
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(flush)
{
	glFlush();
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(framebufferRenderbuffer)
{
	glFramebufferRenderbuffer((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), (GLenum)V2N(arguments[2]), (GLuint)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(framebufferTexture2D)
{
	glFramebufferTexture2D((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), (GLenum)V2N(arguments[2]), (GLuint)V2N(arguments[3]), (GLint)V2N(arguments[4]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(frontFace)
{
	glFrontFace((GLenum)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(createBuffer)
{
	GLuint buf = 0;
	glGenBuffers(1, &buf);
	return N2V(buf);
}

WEBGL_FUNCTION_HEAD(generateMipmap)
{
	glGenerateMipmap((GLenum)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(createFramebuffer)
{
	GLuint buf = 0;
	glGenFramebuffers(1, &buf);
	return N2V(buf);
}

WEBGL_FUNCTION_HEAD(createRenderbuffer)
{
	GLuint buf = 0;
	glGenRenderbuffers(1, &buf);
	return N2V(buf);
}

WEBGL_FUNCTION_HEAD(createTexture)
{
	GLuint tex = 0;
	glGenTextures(1, &tex);
	return N2V(tex);
}

WEBGL_FUNCTION_HEAD(getActiveAttrib)
{
	JSStringRef jname;
	JSStringRef name;
	JSObjectRef obj = JSObjectMake(ctx, NULL, NULL);
	GLsizei length = 0;
	GLint size = 0;
	GLenum type = 0;
	GLchar aname[MAX_INFO_NAMELEN + 1];

	glGetActiveAttrib((GLuint)V2N(arguments[0]), (GLuint)V2N(arguments[1]), MAX_INFO_NAMELEN, &length, &size, &type, aname);

	name = JSStringCreateWithUTF8CString("type");
	JSObjectSetProperty(ctx, obj, name, N2V(type), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(name);

	name = JSStringCreateWithUTF8CString("size");
	JSObjectSetProperty(ctx, obj, name, N2V(size), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(name);

	jname = JSStringCreateWithUTF8CString(aname);
	name = JSStringCreateWithUTF8CString("name");
	JSObjectSetProperty(ctx, obj, name, JSValueMakeString(ctx, jname), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(name);

	return obj;
}

WEBGL_FUNCTION_HEAD(getActiveUniform)
{
	JSStringRef jname;
	JSStringRef name;
	JSObjectRef obj = JSObjectMake(ctx, NULL, NULL);
	GLsizei length = 0;
	GLint size = 0;
	GLenum type = 0;
	GLchar aname[MAX_INFO_NAMELEN + 1];

	glGetActiveUniform((GLuint)V2N(arguments[0]), (GLuint)V2N(arguments[1]), MAX_INFO_NAMELEN, &length, &size, &type, aname);

	name = JSStringCreateWithUTF8CString("type");
	JSObjectSetProperty(ctx, obj, name, N2V(type), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(name);

	name = JSStringCreateWithUTF8CString("size");
	JSObjectSetProperty(ctx, obj, name, N2V(size), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(name);

	jname = JSStringCreateWithUTF8CString(aname);
	name = JSStringCreateWithUTF8CString("name");
	JSObjectSetProperty(ctx, obj, name, JSValueMakeString(ctx, jname), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(name);

	return obj;
}

WEBGL_FUNCTION_HEAD(getAttachedShaders)
{
	JSObjectRef ret;
	JSValueRef jshaders[MAX_ATTACHED_SHADERS];
	GLuint shaders[MAX_ATTACHED_SHADERS];
	uint32_t scount = 0;

	glGetAttachedShaders((GLuint)V2N(arguments[0]), sizeof(shaders) / sizeof(GLuint), &scount, shaders);
	if (!scount)
		return JSValueMakeNull(ctx);

	for (int i = 0; i < scount; i++) {
		jshaders[i] = N2V(shaders[i]);
	}

	return JSObjectMakeArray(ctx, scount, jshaders, exception);
}

WEBGL_FUNCTION_HEAD(getAttribLocation)
{
	char *buffer = NULL;
	size_t sz = 0;

	JSStringRef str = JSValueToStringCopy(ctx, arguments[1], exception);
	sz = JSStringGetMaximumUTF8CStringSize(str);
	buffer = malloc(sz);
	JSStringGetUTF8CString(str, buffer, sz);
	JSStringRelease(str);

	int ret = glGetAttribLocation((GLenum)V2N(arguments[0]), buffer);
	free(buffer);

	return N2V(ret);
}

WEBGL_FUNCTION_HEAD(getBufferParameter)
{
	GLint params;
	glGetBufferParameteriv((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), &params);
	return N2V(params);
}

WEBGL_FUNCTION_HEAD(getContextAttributes)
{
	printf("[webgl] Unimplemented API call: %s\n", __FUNCTION__);
	abort();
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(getError)
{
	return N2V(glGetError());
}

WEBGL_FUNCTION_HEAD(getExtension)
{
	printf("[webgl] Unimplemented API call: %s\n", __FUNCTION__);
	abort();
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(getFramebufferAttachmentParameter)
{
	GLint params;
	glGetFramebufferAttachmentParameteriv((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), (GLenum)V2N(arguments[2]), &params);
	return N2V(params);
}

WEBGL_FUNCTION_HEAD(getParameter)
{
	printf("[webgl] Unimplemented API call: %s\n", __FUNCTION__);
	abort();
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(getProgramInfoLog)
{
	GLsizei length = 0;
	GLchar log[MAX_PROGRAM_INFO_LOG_LEN + 1];

	glGetProgramInfoLog((GLenum)V2N(arguments[0]), MAX_PROGRAM_INFO_LOG_LEN, &length, log);
	return JSValueMakeString(ctx, JSStringCreateWithUTF8CString(log));
}

WEBGL_FUNCTION_HEAD(getProgramParameter)
{
	GLint params;
	GLenum pname = (GLenum)V2N(arguments[1]);

	glGetProgramiv((GLuint)V2N(arguments[0]), pname, &params);

	switch (pname) {
	case GL_DELETE_STATUS:
	case GL_LINK_STATUS:
	case GL_VALIDATE_STATUS:
		return params ? JSValueMakeBoolean(ctx, true) : JSValueMakeBoolean(ctx, false);
	default:
		break;
	}

	return N2V(params);
}

WEBGL_FUNCTION_HEAD(getRenderbufferParameter)
{
	GLint params;
	glGetRenderbufferParameteriv((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), &params);
	return N2V(params);
}

WEBGL_FUNCTION_HEAD(getShaderParameter)
{
	GLint params;
	GLenum pname = (GLenum)V2N(arguments[1]);

	glGetShaderiv((GLenum)V2N(arguments[0]), pname, &params);

	switch (pname) {
	case GL_DELETE_STATUS:
	case GL_COMPILE_STATUS:
		return params ? JSValueMakeBoolean(ctx, true) : JSValueMakeBoolean(ctx, false);
	default:
		break;
	}

	return N2V(params);
}

WEBGL_FUNCTION_HEAD(getShaderInfoLog)
{
	GLsizei length = 0;
	GLchar log[MAX_SHADER_INFO_LOG_LEN + 1];

	glGetShaderInfoLog((GLenum)V2N(arguments[0]), MAX_SHADER_INFO_LOG_LEN, &length, log);
	return JSValueMakeString(ctx, JSStringCreateWithUTF8CString(log));
}

WEBGL_FUNCTION_HEAD(getShaderPrecisionFormat)
{
	JSStringRef name;
	JSObjectRef obj = JSObjectMake(ctx, NULL, NULL);

	GLint range[2];
	GLint precision;
	
	glGetShaderPrecisionFormat((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), range, &precision);

	name = JSStringCreateWithUTF8CString("rangeMin");
	JSObjectSetProperty(ctx, obj, name, N2V(range[0]), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(name);

	name = JSStringCreateWithUTF8CString("rangeMax");
	JSObjectSetProperty(ctx, obj, name, N2V(range[1]), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(name);

	name = JSStringCreateWithUTF8CString("precision");
	JSObjectSetProperty(ctx, obj, name, N2V(precision), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(name);

	return obj;
}

WEBGL_FUNCTION_HEAD(getShaderSource)
{
	GLint buflen = 0;
	GLchar *buf = NULL;
	GLuint shader = (GLuint)V2N(arguments[0]);

	glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &buflen);
	if (!buflen)
		return JSValueMakeNull(ctx);

	buf = malloc(buflen);
	if (!buf)
		return JSValueMakeNull(ctx);

	glGetShaderSource(shader, buflen, NULL, buf);

	JSValueRef ret = JSValueMakeString(ctx, JSStringCreateWithUTF8CString(buf));
	free(buf);

	return ret;
}

WEBGL_FUNCTION_HEAD(getSupportedExtensions)
{
	printf("[webgl] Unimplemented API call: %s\n", __FUNCTION__);
	abort();
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(getTexParameter)
{
	GLint params;
	glGetTexParameteriv((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), &params);
	return N2V(params);
}

WEBGL_FUNCTION_HEAD(getUniform)
{
	GLuint program_shader = (GLuint)V2N(arguments[0]);
	GLint location = (GLint)V2N(arguments[0]);
	GLint i, j, activeUniforms = 0;
	GLsizei len;
	GLint size, length;
	GLenum type, base_type;
	bool found = false;
	GLfloat values_f[16];
	GLint values_i[4];
	GLuint values_ui[4];
	JSObjectRef res;
	JSValueRef val[16];
	char name[1024], sname[1150];

	//openGL doesn't provide a way to get a uniform type from its location
	//we need to browse all active uniforms by name in the program
	//then look for the location of each uniform and check against the desired location

	glGetProgramiv(program_shader, GL_ACTIVE_UNIFORMS, &activeUniforms);
	for (i = 0; i < activeUniforms; i++) {
		glGetActiveUniform(program_shader, i, 1024, &len, &size, &type, name);
		if ((len > 3) && !strcmp(name + len - 3, "[0]")) {
			len -= 3;
			name[len] = 0;
		}

		for (j = 0; j < size; j++) {
			strcpy(sname, name);
			if ((size > 1) && (j >= 1)) {
				char szIdx[100];
				sprintf(szIdx, "[%d]", j);
				strcat(sname, szIdx);
			}
			GLint loc = glGetUniformLocation(program_shader, sname);
			if (loc == location) {
				found = true;
				break;
			}
		}
		if (found) break;
	}

	if (!found) {
		return JSValueMakeUndefined(ctx);
	}

	switch (type) {
	case GL_BOOL: base_type = GL_BOOL; length = 1; break;
	case GL_BOOL_VEC2: base_type = GL_BOOL; length = 2; break;
	case GL_BOOL_VEC3: base_type = GL_BOOL; length = 3; break;
	case GL_BOOL_VEC4: base_type = GL_BOOL; length = 4; break;
	case GL_INT: base_type = GL_INT; length = 1; break;
	case GL_INT_VEC2: base_type = GL_INT; length = 2; break;
	case GL_INT_VEC3: base_type = GL_INT; length = 3; break;
	case GL_INT_VEC4: base_type = GL_INT; length = 4; break;
	case GL_FLOAT: base_type = GL_FLOAT; length = 1; break;
	case GL_FLOAT_VEC2: base_type = GL_FLOAT; length = 2; break;
	case GL_FLOAT_VEC3: base_type = GL_FLOAT; length = 3; break;
	case GL_FLOAT_VEC4: base_type = GL_FLOAT; length = 4; break;
	case GL_FLOAT_MAT2: base_type = GL_FLOAT; length = 4; break;
	case GL_FLOAT_MAT3: base_type = GL_FLOAT; length = 9; break;
	case GL_FLOAT_MAT4: base_type = GL_FLOAT; length = 16; break;
	case GL_SAMPLER_2D:
	case GL_SAMPLER_CUBE:
		base_type = GL_INT;
		length = 1;
		break;
	default:
		return JSValueMakeUndefined(ctx);
	}

#define RETURN_SINGLE_OR_ARRAY(__arr) \
		if (length == 1) return JSValueMakeNumber(ctx, __arr[0]);\
		for (i=0; i<length; i++)\
			val[i] = JSValueMakeNumber(ctx, __arr[i]);\
		return JSObjectMakeArray(ctx, length, val, exception);

	switch (base_type) {
	case GL_FLOAT:
		glGetUniformfv(program_shader, location, values_f);
		RETURN_SINGLE_OR_ARRAY(values_f);

	case GL_INT:
		glGetUniformiv(program_shader, location, values_i);
		RETURN_SINGLE_OR_ARRAY(values_i);

	case GL_UNSIGNED_INT:
		glGetUniformiv(program_shader, location, values_ui);
		RETURN_SINGLE_OR_ARRAY(values_ui);

	case GL_BOOL:
		glGetUniformiv(program_shader, location, values_i);
		RETURN_SINGLE_OR_ARRAY(values_i);

	default:
		break;
	}

#undef RETURN_GLANY_SINGLE_OR_ARRAY

	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(getUniformLocation)
{
	char *buffer = NULL;
	size_t sz = 0;

	JSStringRef str = JSValueToStringCopy(ctx, arguments[1], exception);
	sz = JSStringGetMaximumUTF8CStringSize(str);
	buffer = malloc(sz);
	JSStringGetUTF8CString(str, buffer, sz);
	JSStringRelease(str);

	int ret = glGetUniformLocation((GLenum)V2N(arguments[0]), buffer);
	free(buffer);

	return N2V(ret);
}

WEBGL_FUNCTION_HEAD(getVertexAttrib)
{
	uint32_t i, count;
	GLuint index = (GLuint)V2N(arguments[0]);
	GLenum pname = (GLenum)V2N(arguments[1]);
	float floats[4];
	int32_t ints[4];

	switch (pname) {
	case GL_CURRENT_VERTEX_ATTRIB:
		glGetVertexAttribfv(index, pname, floats);
		JSValueRef val[4];
		for (i = 0; i < 4; i++)
			val[i] = JSValueMakeNumber(ctx, floats[i]);
		return JSObjectMakeArray(ctx, 4, val, exception);
	case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
	case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
		glGetVertexAttribiv(index, pname, ints);
		return ints[0] ? JSValueMakeBoolean(ctx, true) : JSValueMakeBoolean(ctx, false);
	case GL_VERTEX_ATTRIB_ARRAY_SIZE:
	case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
	case GL_VERTEX_ATTRIB_ARRAY_TYPE:
	case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
		glGetVertexAttribiv(index, pname, ints);
		return N2V(ints[0]);
	}

	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(getVertexAttribOffset)
{
	void *ptr = NULL;
	glGetVertexAttribPointerv((GLuint)V2N(arguments[0]), (GLenum)V2N(arguments[1]), &ptr);
	return N2V(ptr);
}

WEBGL_FUNCTION_HEAD(hint)
{
	glHint((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(isBuffer)
{
	return JSValueMakeBoolean(ctx, glIsBuffer((GLuint)V2N(arguments[0])));
}

WEBGL_FUNCTION_HEAD(isContextLost)
{
	return JSValueMakeBoolean(ctx, false);
}

WEBGL_FUNCTION_HEAD(isEnabled)
{
	return JSValueMakeBoolean(ctx, glIsEnabled((GLenum)V2N(arguments[0])));
}

WEBGL_FUNCTION_HEAD(isFramebuffer)
{
	return JSValueMakeBoolean(ctx, glIsFramebuffer((GLuint)V2N(arguments[0])));
}

WEBGL_FUNCTION_HEAD(isProgram)
{
	return JSValueMakeBoolean(ctx, glIsProgram((GLuint)V2N(arguments[0])));
}

WEBGL_FUNCTION_HEAD(isRenderbuffer)
{
	return JSValueMakeBoolean(ctx, glIsRenderbuffer((GLuint)V2N(arguments[0])));
}

WEBGL_FUNCTION_HEAD(isShader)
{
	return JSValueMakeBoolean(ctx, glIsShader((GLuint)V2N(arguments[0])));
}

WEBGL_FUNCTION_HEAD(isTexture)
{
	return JSValueMakeBoolean(ctx, glIsTexture((GLuint)V2N(arguments[0])));
}

WEBGL_FUNCTION_HEAD(lineWidth)
{
	glLineWidth((GLfloat)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(linkProgram)
{
	glLinkProgram((GLuint)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(makeXRCompatible)
{
	printf("[webgl] Unimplemented API call: %s\n", __FUNCTION__);
	abort();
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(pixelStorei)
{
	glPixelStorei((GLenum)V2N(arguments[0]), (GLint)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(polygonOffset)
{
	glPolygonOffset((GLfloat)V2N(arguments[0]), (GLfloat)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(readPixels)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[6], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glReadPixels((GLint)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLsizei)V2N(arguments[2]), (GLsizei)V2N(arguments[3]), (GLenum)V2N(arguments[4]), (GLenum)V2N(arguments[5]), priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(releaseShaderCompiler)
{
	glReleaseShaderCompiler();
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(renderbufferStorage)
{
	glRenderbufferStorage((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), (GLsizei)V2N(arguments[2]), (GLsizei)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(sampleCoverage)
{
	glSampleCoverage((GLclampf)V2N(arguments[0]), (GLboolean)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(scissor)
{
	glScissor((GLint)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLsizei)V2N(arguments[2]), (GLsizei)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(shaderBinary)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[2], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glShaderBinary(1, (GLuint)V2N(arguments[0]), (GLenum)V2N(arguments[1]), priv->data, (GLsizei)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(shaderSource)
{
	char *buffer = NULL;
	size_t sz = 0;

	JSStringRef str = JSValueToStringCopy(ctx, arguments[1], exception);
	sz = JSStringGetMaximumUTF8CStringSize(str);
	buffer = malloc(sz);
	JSStringGetUTF8CString(str, buffer, sz);
	JSStringRelease(str);
	sz = strnlen_s(buffer, sz);

	glShaderSource((GLuint)V2N(arguments[0]), 1, &buffer, &sz);
	free(buffer);

	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(stencilFunc)
{
	glStencilFunc((GLenum)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLuint)V2N(arguments[2]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(stencilFuncSeparate)
{
	glStencilFuncSeparate((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), (GLint)V2N(arguments[2]), (GLuint)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(stencilMask)
{
	glStencilMask((GLuint)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(stencilMaskSeparate)
{
	glStencilMaskSeparate((GLenum)V2N(arguments[0]), (GLuint)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(stencilOp)
{
	glStencilOp((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), (GLenum)V2N(arguments[2]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(stencilOpSeparate)
{
	glStencilOpSeparate((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), (GLenum)V2N(arguments[2]), (GLenum)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(texImage2D)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[8], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glTexImage2D((GLenum)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLint)V2N(arguments[2]), (GLsizei)V2N(arguments[3]), (GLsizei)V2N(arguments[4]), (GLint)V2N(arguments[5]), (GLenum)V2N(arguments[6]), (GLenum)V2N(arguments[7]), priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(texParameterf)
{
	glTexParameterf((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), (GLfloat)V2N(arguments[2]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(texParameteri)
{
	glTexParameteri((GLenum)V2N(arguments[0]), (GLenum)V2N(arguments[1]), (GLint)V2N(arguments[2]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(texSubImage2D)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[8], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glTexSubImage2D((GLenum)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLint)V2N(arguments[2]), (GLint)V2N(arguments[3]), (GLsizei)V2N(arguments[4]), (GLsizei)V2N(arguments[5]), (GLenum)V2N(arguments[6]), (GLenum)V2N(arguments[7]), priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform1f)
{
	glUniform1f((GLint)V2N(arguments[0]), (GLfloat)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform1fv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[1], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glUniform1fv((GLint)V2N(arguments[0]), 1, priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform1i)
{
	glUniform1i((GLint)V2N(arguments[0]), (GLint)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform1iv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[1], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glUniform1iv((GLint)V2N(arguments[0]), 1, priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform2f)
{
	glUniform2f((GLint)V2N(arguments[0]), (GLfloat)V2N(arguments[1]), (GLfloat)V2N(arguments[2]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform2fv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[1], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glUniform2fv((GLint)V2N(arguments[0]), 1, priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform2i)
{
	glUniform2i((GLint)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLint)V2N(arguments[2]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform2iv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[1], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glUniform2iv((GLint)V2N(arguments[0]), 1, priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform3f)
{
	glUniform3f((GLint)V2N(arguments[0]), (GLfloat)V2N(arguments[1]), (GLfloat)V2N(arguments[2]), (GLfloat)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform3fv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[1], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glUniform3fv((GLint)V2N(arguments[0]), 1, priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform3i)
{
	glUniform3i((GLint)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLint)V2N(arguments[2]), (GLint)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform3iv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[1], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glUniform3iv((GLint)V2N(arguments[0]), 1, priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform4f)
{
	glUniform4f((GLint)V2N(arguments[0]), (GLfloat)V2N(arguments[1]), (GLfloat)V2N(arguments[2]), (GLfloat)V2N(arguments[3]), (GLfloat)V2N(arguments[4]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform4fv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[1], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glUniform4fv((GLint)V2N(arguments[0]), 1, priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform4i)
{
	glUniform4i((GLint)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLint)V2N(arguments[2]), (GLint)V2N(arguments[3]), (GLint)V2N(arguments[4]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniform4iv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[1], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glUniform4iv((GLint)V2N(arguments[0]), 1, priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniformMatrix2fv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[2], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glUniformMatrix2fv((GLint)V2N(arguments[0]), 1, (GLboolean)V2N(arguments[1]), priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniformMatrix3fv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[2], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glUniformMatrix3fv((GLint)V2N(arguments[0]), 1, (GLboolean)V2N(arguments[1]), priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(uniformMatrix4fv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[2], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glUniformMatrix4fv((GLint)V2N(arguments[0]), 1, (GLboolean)V2N(arguments[1]), priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(useProgram)
{
	glUseProgram((GLuint)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(validateProgram)
{
	glValidateProgram((GLuint)V2N(arguments[0]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(vertexAttrib1f)
{
	glVertexAttrib1f((GLuint)V2N(arguments[0]), (GLfloat)V2N(arguments[1]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(vertexAttrib1fv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[2], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glVertexAttrib1fv((GLuint)V2N(arguments[0]), priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(vertexAttrib2f)
{
	glVertexAttrib2f((GLuint)V2N(arguments[0]), (GLfloat)V2N(arguments[1]), (GLfloat)V2N(arguments[2]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(vertexAttrib2fv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[2], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glVertexAttrib2fv((GLuint)V2N(arguments[0]), priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(vertexAttrib3f)
{
	glVertexAttrib3f((GLuint)V2N(arguments[0]), (GLfloat)V2N(arguments[1]), (GLfloat)V2N(arguments[2]), (GLfloat)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(vertexAttrib3fv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[2], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glVertexAttrib3fv((GLuint)V2N(arguments[0]), priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(vertexAttrib4f)
{
	glVertexAttrib4f((GLuint)V2N(arguments[0]), (GLfloat)V2N(arguments[1]), (GLfloat)V2N(arguments[2]), (GLfloat)V2N(arguments[3]), (GLfloat)V2N(arguments[4]));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(vertexAttrib4fv)
{
	LIExtTypedArrayPriv *priv = (LIExtTypedArrayPriv *)JSObjectGetPrivate(JSValueToObject(ctx, arguments[2], exception));
	if (priv == NULL) {
		PRINT_ARRAY_WARNING
		abort();
	}

	glVertexAttrib4fv((GLuint)V2N(arguments[0]), priv->data);
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(vertexAttribPointer)
{
	glVertexAttribPointer((GLuint)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLenum)V2N(arguments[2]), (GLboolean)V2N(arguments[3]), (GLsizei)V2N(arguments[4]), (GLvoid*)(int64_t)(V2N(arguments[5])));
	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(viewport)
{
	glViewport((GLint)V2N(arguments[0]), (GLint)V2N(arguments[1]), (GLsizei)V2N(arguments[2]), (GLsizei)V2N(arguments[3]));
	return JSValueMakeUndefined(ctx);
}

bool getConfigUint32(char *config, char *name, uint32_t *val)
{
	char *end = NULL;
	char *pos = strstr(config, name);
	if (!pos) {
		*val = 0;
		return false;
	}

	pos += strlen(name) + 1;

	*val = strtoul(pos, &end, 10);
	return true;
}

WEBGL_FUNCTION_HEAD(createContext)
{
	SceUID cfd = SCE_UID_INVALID_UID;
	char *config = NULL;
	uint32_t val = 0;
	EGLContext context;
	EGLConfig configs[2];
	EGLBoolean eRetStatus;
	EGLint major, minor;
	EGLint config_count;
	PVRSRV_PSP2_APPHINT hint;
	Psp2NativeWindow win;

	if (argumentCount == 3) {
		JSStringRef str = JSValueToStringCopy(ctx, arguments[2], exception);
		size_t sz = JSStringGetMaximumUTF8CStringSize(str);
		void *buffer = malloc(sz);
		JSStringGetUTF8CString(str, buffer, sz);
		JSStringRelease(str);

		cfd = sceIoOpen(buffer, SCE_O_RDONLY, 0);
		free(buffer);

		if (cfd <= 0) {
			printf("[webgl] Apphint file not found, using defaults\n");
		}
		else {
			SceIoStat st;
			sceIoGetstatByFd(cfd, &st);

			config = (char *)malloc(st.st_size);

			sceIoRead(cfd, config, st.st_size);
			sceIoClose(cfd);
		}
	}

	win.windowSize = V2N(arguments[0]);

	PVRSRVInitializeAppHint(&hint);

	if (cfd > 0) {
		if (getConfigUint32(config, "ui32UNCTexHeapSize", &val))
			hint.ui32UNCTexHeapSize = val;
		if (getConfigUint32(config, "ui32CDRAMTexHeapSize", &val))
			hint.ui32CDRAMTexHeapSize = val;
		if (getConfigUint32(config, "ui32SwTexOpThreadPriority", &val))
			hint.ui32SwTexOpThreadPriority = val;
		if (getConfigUint32(config, "ui32SwTexOpThreadAffinity", &val))
			hint.ui32SwTexOpThreadAffinity = val;
		if (getConfigUint32(config, "ui32SwTexOpCleanupDelay", &val))
			hint.ui32SwTexOpCleanupDelay = val;
		if (getConfigUint32(config, "ui32ExternalZBufferMode", &val))
			hint.ui32ExternalZBufferMode = val;
		if (getConfigUint32(config, "ui32ExternalZBufferXSize", &val))
			hint.ui32ExternalZBufferXSize = val;
		if (getConfigUint32(config, "ui32ExternalZBufferYSize", &val))
			hint.ui32ExternalZBufferYSize = val;
	}

	PVRSRVCreateVirtualAppHint(&hint);

	dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eRetStatus = eglInitialize(dpy, &major, &minor);

	if (eRetStatus != EGL_TRUE)
	{
		printf("[webgl] eglInitialize error\n");
		abort();
	}

	eRetStatus = eglGetConfigs(dpy, configs, 2, &config_count);

	if (eRetStatus != EGL_TRUE)
	{
		printf("[webgl] eglGetConfigs error\n");
		abort();
	}

	if (argumentCount > 1) {
		JSObjectRef jarr;
		JSValueRef jlen;
		JSStringRef jls;
		JSValueRef pres;
		size_t configCount;
		EGLint cfg_attribs[36];

		jarr = JSValueToObject(ctx, arguments[1], exception);
		jls = JSStringCreateWithUTF8CString("length");
		jlen = JSObjectGetProperty(ctx, jarr, jls, exception);
		JSStringRelease(jls);
		configCount = (size_t)V2N(jlen);

		for (int i = 0; i < configCount; i++) {
			pres = JSObjectGetPropertyAtIndex(ctx, jarr, i, exception);
			cfg_attribs[i] = (EGLint)V2N(pres);
		}

		eRetStatus = eglChooseConfig(dpy, cfg_attribs, configs, 2, &config_count);
	}
	else {
		EGLint cfg_attribs[] = { EGL_BUFFER_SIZE,    EGL_DONT_CARE,
								EGL_RED_SIZE,       8,
								EGL_GREEN_SIZE,     8,
								EGL_BLUE_SIZE,      8,
								EGL_ALPHA_SIZE,		8,
								EGL_DEPTH_SIZE,		16,
								EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
								EGL_CONFORMANT, EGL_OPENGL_ES2_BIT,
								EGL_NONE };

		eRetStatus = eglChooseConfig(dpy, cfg_attribs, configs, 2, &config_count);
	}

	if (eRetStatus != EGL_TRUE)
	{
		printf("[webgl] eglChooseConfig error\n");
		abort();
	}

	win.type = PSP2_DRAWABLE_TYPE_WINDOW;
	win.numFlipBuffers = 2;
	win.flipChainThrdAffinity = SCE_KERNEL_CPU_MASK_USER_0;

	if (cfd > 0) {
		if (getConfigUint32(config, "numFlipBuffers", &val))
			win.numFlipBuffers = val;
		if (getConfigUint32(config, "flipChainThrdAffinity", &val))
			win.flipChainThrdAffinity = val;
	}

	switch (win.windowSize) {
	case PSP2_WINDOW_960X544:
		s_width = 960;
		s_height = 544;
		break;
	case PSP2_WINDOW_480X272:
		s_width = 480;
		s_height = 272;
		break;
	case PSP2_WINDOW_640X368:
		s_width = 640;
		s_height = 368;
		break;
	case PSP2_WINDOW_720X408:
		s_width = 720;
		s_height = 408;
		break;
	case PSP2_WINDOW_1280X725:
		s_width = 1280;
		s_height = 725;
		break;
	case PSP2_WINDOW_1920X1088:
		s_width = 1920;
		s_height = 1088;
		break;
	}

	surface = eglCreateWindowSurface(dpy, configs[0], &win, NULL);

	if (surface == EGL_NO_SURFACE)
	{
		printf("[webgl] eglCreateWindowSurface error\n");
		abort();
	}

	EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_CONTEXT_PRIORITY_LEVEL_IMG, EGL_CONTEXT_PRIORITY_HIGH_IMG, EGL_NONE };

	context = eglCreateContext(dpy, configs[0], EGL_NO_CONTEXT, context_attribs);

	if (context == EGL_NO_CONTEXT)
	{
		printf("[webgl] eglCreateContext error\n");
		abort();
	}

	eRetStatus = eglMakeCurrent(dpy, surface, surface, context);

	if (eRetStatus != EGL_TRUE)
	{
		printf("[webgl] eglMakeCurrent error\n");
		abort();
	}

	if (config)
		free(config);

	JSStringRef name;
	name = JSStringCreateWithUTF8CString("drawingBufferWidth");
	JSObjectSetProperty(ctx, thisObject, name, N2V(s_width), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(name);

	name = JSStringCreateWithUTF8CString("drawingBufferHeight");
	JSObjectSetProperty(ctx, thisObject, name, N2V(s_height), kJSPropertyAttributeReadOnly, exception);
	JSStringRelease(name);

	return JSValueMakeUndefined(ctx);
}

WEBGL_FUNCTION_HEAD(commit)
{
	eglSwapBuffers(dpy, surface);
	return JSValueMakeUndefined(ctx);
}

void webglAddFunctionProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp)
{
	JSStringRef name;
	JSObjectRef prop;

	SET_FUNCTION_PROPERTY(activeTexture)
	SET_FUNCTION_PROPERTY(attachShader)
	SET_FUNCTION_PROPERTY(bindAttribLocation)
	SET_FUNCTION_PROPERTY(bindBuffer)
	SET_FUNCTION_PROPERTY(bindFramebuffer)
	SET_FUNCTION_PROPERTY(bindRenderbuffer)
	SET_FUNCTION_PROPERTY(bindTexture)
	SET_FUNCTION_PROPERTY(blendColor)
	SET_FUNCTION_PROPERTY(blendEquation)
	SET_FUNCTION_PROPERTY(blendEquationSeparate)
	SET_FUNCTION_PROPERTY(blendFunc)
	SET_FUNCTION_PROPERTY(blendFuncSeparate)
	SET_FUNCTION_PROPERTY(bufferData)
	SET_FUNCTION_PROPERTY(bufferSubData)
	SET_FUNCTION_PROPERTY(checkFramebufferStatus)
	SET_FUNCTION_PROPERTY(clear)
	SET_FUNCTION_PROPERTY(clearColor)
	SET_FUNCTION_PROPERTY(clearDepth)
	SET_FUNCTION_PROPERTY(clearStencil)
	SET_FUNCTION_PROPERTY(colorMask)
	SET_FUNCTION_PROPERTY(compileShader)
	SET_FUNCTION_PROPERTY(compressedTexImage2D)
	SET_FUNCTION_PROPERTY(compressedTexSubImage2D)
	SET_FUNCTION_PROPERTY(copyTexImage2D)
	SET_FUNCTION_PROPERTY(copyTexSubImage2D)
	SET_FUNCTION_PROPERTY(createProgram)
	SET_FUNCTION_PROPERTY(createShader)
	SET_FUNCTION_PROPERTY(cullFace)
	SET_FUNCTION_PROPERTY(deleteBuffer)
	SET_FUNCTION_PROPERTY(deleteFramebuffer)
	SET_FUNCTION_PROPERTY(deleteProgram)
	SET_FUNCTION_PROPERTY(deleteRenderbuffer)
	SET_FUNCTION_PROPERTY(deleteShader)
	SET_FUNCTION_PROPERTY(deleteTexture)
	SET_FUNCTION_PROPERTY(depthFunc)
	SET_FUNCTION_PROPERTY(depthMask)
	SET_FUNCTION_PROPERTY(depthRange)
	SET_FUNCTION_PROPERTY(detachShader)
	SET_FUNCTION_PROPERTY(disable)
	SET_FUNCTION_PROPERTY(disableVertexAttribArray)
	SET_FUNCTION_PROPERTY(drawArrays)
	SET_FUNCTION_PROPERTY(drawElements)
	SET_FUNCTION_PROPERTY(enable)
	SET_FUNCTION_PROPERTY(enableVertexAttribArray)
	SET_FUNCTION_PROPERTY(finish)
	SET_FUNCTION_PROPERTY(flush)
	SET_FUNCTION_PROPERTY(framebufferRenderbuffer)
	SET_FUNCTION_PROPERTY(framebufferTexture2D)
	SET_FUNCTION_PROPERTY(frontFace)
	SET_FUNCTION_PROPERTY(createBuffer)
	SET_FUNCTION_PROPERTY(generateMipmap)
	SET_FUNCTION_PROPERTY(createFramebuffer)
	SET_FUNCTION_PROPERTY(createRenderbuffer)
	SET_FUNCTION_PROPERTY(createTexture)
	SET_FUNCTION_PROPERTY(getActiveAttrib)
	SET_FUNCTION_PROPERTY(getActiveUniform)
	SET_FUNCTION_PROPERTY(getAttachedShaders)
	SET_FUNCTION_PROPERTY(getAttribLocation)
	SET_FUNCTION_PROPERTY(getBufferParameter)
	SET_FUNCTION_PROPERTY(getContextAttributes)
	SET_FUNCTION_PROPERTY(getError)
	SET_FUNCTION_PROPERTY(getExtension)
	SET_FUNCTION_PROPERTY(getFramebufferAttachmentParameter)
	SET_FUNCTION_PROPERTY(getParameter)
	SET_FUNCTION_PROPERTY(getProgramInfoLog)
	SET_FUNCTION_PROPERTY(getProgramParameter)
	SET_FUNCTION_PROPERTY(getRenderbufferParameter)
	SET_FUNCTION_PROPERTY(getShaderParameter)
	SET_FUNCTION_PROPERTY(getShaderInfoLog)
	SET_FUNCTION_PROPERTY(getShaderPrecisionFormat)
	SET_FUNCTION_PROPERTY(getShaderSource)
	SET_FUNCTION_PROPERTY(getSupportedExtensions)
	SET_FUNCTION_PROPERTY(getTexParameter)
	SET_FUNCTION_PROPERTY(getUniform)
	SET_FUNCTION_PROPERTY(getUniformLocation)
	SET_FUNCTION_PROPERTY(getVertexAttrib)
	SET_FUNCTION_PROPERTY(getVertexAttribOffset)
	SET_FUNCTION_PROPERTY(hint)
	SET_FUNCTION_PROPERTY(isBuffer)
	SET_FUNCTION_PROPERTY(isContextLost)
	SET_FUNCTION_PROPERTY(isEnabled)
	SET_FUNCTION_PROPERTY(isFramebuffer)
	SET_FUNCTION_PROPERTY(isProgram)
	SET_FUNCTION_PROPERTY(isRenderbuffer)
	SET_FUNCTION_PROPERTY(isShader)
	SET_FUNCTION_PROPERTY(isTexture)
	SET_FUNCTION_PROPERTY(lineWidth)
	SET_FUNCTION_PROPERTY(linkProgram)
	SET_FUNCTION_PROPERTY(makeXRCompatible)
	SET_FUNCTION_PROPERTY(pixelStorei)
	SET_FUNCTION_PROPERTY(polygonOffset)
	SET_FUNCTION_PROPERTY(readPixels)
	SET_FUNCTION_PROPERTY(releaseShaderCompiler)
	SET_FUNCTION_PROPERTY(renderbufferStorage)
	SET_FUNCTION_PROPERTY(sampleCoverage)
	SET_FUNCTION_PROPERTY(scissor)
	SET_FUNCTION_PROPERTY(shaderBinary)
	SET_FUNCTION_PROPERTY(shaderSource)
	SET_FUNCTION_PROPERTY(stencilFunc)
	SET_FUNCTION_PROPERTY(stencilFuncSeparate)
	SET_FUNCTION_PROPERTY(stencilMask)
	SET_FUNCTION_PROPERTY(stencilMaskSeparate)
	SET_FUNCTION_PROPERTY(stencilOp)
	SET_FUNCTION_PROPERTY(stencilOpSeparate)
	SET_FUNCTION_PROPERTY(texImage2D)
	SET_FUNCTION_PROPERTY(texParameterf)
	SET_FUNCTION_PROPERTY(texParameteri)
	SET_FUNCTION_PROPERTY(texSubImage2D)
	SET_FUNCTION_PROPERTY(uniform1f)
	SET_FUNCTION_PROPERTY(uniform1fv)
	SET_FUNCTION_PROPERTY(uniform1i)
	SET_FUNCTION_PROPERTY(uniform1iv)
	SET_FUNCTION_PROPERTY(uniform2f)
	SET_FUNCTION_PROPERTY(uniform2fv)
	SET_FUNCTION_PROPERTY(uniform2i)
	SET_FUNCTION_PROPERTY(uniform2iv)
	SET_FUNCTION_PROPERTY(uniform3f)
	SET_FUNCTION_PROPERTY(uniform3fv)
	SET_FUNCTION_PROPERTY(uniform3i)
	SET_FUNCTION_PROPERTY(uniform3iv)
	SET_FUNCTION_PROPERTY(uniform4f)
	SET_FUNCTION_PROPERTY(uniform4fv)
	SET_FUNCTION_PROPERTY(uniform4i)
	SET_FUNCTION_PROPERTY(uniform4iv)
	SET_FUNCTION_PROPERTY(uniformMatrix2fv)
	SET_FUNCTION_PROPERTY(uniformMatrix3fv)
	SET_FUNCTION_PROPERTY(uniformMatrix4fv)
	SET_FUNCTION_PROPERTY(useProgram)
	SET_FUNCTION_PROPERTY(validateProgram)
	SET_FUNCTION_PROPERTY(vertexAttrib1f)
	SET_FUNCTION_PROPERTY(vertexAttrib1fv)
	SET_FUNCTION_PROPERTY(vertexAttrib2f)
	SET_FUNCTION_PROPERTY(vertexAttrib2fv)
	SET_FUNCTION_PROPERTY(vertexAttrib3f)
	SET_FUNCTION_PROPERTY(vertexAttrib3fv)
	SET_FUNCTION_PROPERTY(vertexAttrib4f)
	SET_FUNCTION_PROPERTY(vertexAttrib4fv)
	SET_FUNCTION_PROPERTY(vertexAttribPointer)
	SET_FUNCTION_PROPERTY(viewport)
	SET_FUNCTION_PROPERTY(createContext)
	SET_FUNCTION_PROPERTY(commit)
}

void webglRemoveFunctionProperties(JSContextRef ctx, JSObjectRef obj, JSValueRef *excp)
{
	JSStringRef name;

	DELETE_FUNCTION_PROPERTY(activeTexture)
	DELETE_FUNCTION_PROPERTY(attachShader)
	DELETE_FUNCTION_PROPERTY(bindAttribLocation)
	DELETE_FUNCTION_PROPERTY(bindBuffer)
	DELETE_FUNCTION_PROPERTY(bindFramebuffer)
	DELETE_FUNCTION_PROPERTY(bindRenderbuffer)
	DELETE_FUNCTION_PROPERTY(bindTexture)
	DELETE_FUNCTION_PROPERTY(blendColor)
	DELETE_FUNCTION_PROPERTY(blendEquation)
	DELETE_FUNCTION_PROPERTY(blendEquationSeparate)
	DELETE_FUNCTION_PROPERTY(blendFunc)
	DELETE_FUNCTION_PROPERTY(blendFuncSeparate)
	DELETE_FUNCTION_PROPERTY(bufferData)
	DELETE_FUNCTION_PROPERTY(bufferSubData)
	DELETE_FUNCTION_PROPERTY(checkFramebufferStatus)
	DELETE_FUNCTION_PROPERTY(clear)
	DELETE_FUNCTION_PROPERTY(clearColor)
	DELETE_FUNCTION_PROPERTY(clearDepth)
	DELETE_FUNCTION_PROPERTY(clearStencil)
	DELETE_FUNCTION_PROPERTY(colorMask)
	DELETE_FUNCTION_PROPERTY(compileShader)
	DELETE_FUNCTION_PROPERTY(compressedTexImage2D)
	DELETE_FUNCTION_PROPERTY(compressedTexSubImage2D)
	DELETE_FUNCTION_PROPERTY(copyTexImage2D)
	DELETE_FUNCTION_PROPERTY(copyTexSubImage2D)
	DELETE_FUNCTION_PROPERTY(createProgram)
	DELETE_FUNCTION_PROPERTY(createShader)
	DELETE_FUNCTION_PROPERTY(cullFace)
	DELETE_FUNCTION_PROPERTY(deleteBuffer)
	DELETE_FUNCTION_PROPERTY(deleteFramebuffer)
	DELETE_FUNCTION_PROPERTY(deleteProgram)
	DELETE_FUNCTION_PROPERTY(deleteRenderbuffer)
	DELETE_FUNCTION_PROPERTY(deleteShader)
	DELETE_FUNCTION_PROPERTY(deleteTexture)
	DELETE_FUNCTION_PROPERTY(depthFunc)
	DELETE_FUNCTION_PROPERTY(depthMask)
	DELETE_FUNCTION_PROPERTY(depthRange)
	DELETE_FUNCTION_PROPERTY(detachShader)
	DELETE_FUNCTION_PROPERTY(disable)
	DELETE_FUNCTION_PROPERTY(disableVertexAttribArray)
	DELETE_FUNCTION_PROPERTY(drawArrays)
	DELETE_FUNCTION_PROPERTY(drawElements)
	DELETE_FUNCTION_PROPERTY(enable)
	DELETE_FUNCTION_PROPERTY(enableVertexAttribArray)
	DELETE_FUNCTION_PROPERTY(finish)
	DELETE_FUNCTION_PROPERTY(flush)
	DELETE_FUNCTION_PROPERTY(framebufferRenderbuffer)
	DELETE_FUNCTION_PROPERTY(framebufferTexture2D)
	DELETE_FUNCTION_PROPERTY(frontFace)
	DELETE_FUNCTION_PROPERTY(createBuffer)
	DELETE_FUNCTION_PROPERTY(generateMipmap)
	DELETE_FUNCTION_PROPERTY(createFramebuffer)
	DELETE_FUNCTION_PROPERTY(createRenderbuffer)
	DELETE_FUNCTION_PROPERTY(createTexture)
	DELETE_FUNCTION_PROPERTY(getActiveAttrib)
	DELETE_FUNCTION_PROPERTY(getActiveUniform)
	DELETE_FUNCTION_PROPERTY(getAttachedShaders)
	DELETE_FUNCTION_PROPERTY(getAttribLocation)
	DELETE_FUNCTION_PROPERTY(getBufferParameter)
	DELETE_FUNCTION_PROPERTY(getContextAttributes)
	DELETE_FUNCTION_PROPERTY(getError)
	DELETE_FUNCTION_PROPERTY(getExtension)
	DELETE_FUNCTION_PROPERTY(getFramebufferAttachmentParameter)
	DELETE_FUNCTION_PROPERTY(getParameter)
	DELETE_FUNCTION_PROPERTY(getProgramInfoLog)
	DELETE_FUNCTION_PROPERTY(getProgramParameter)
	DELETE_FUNCTION_PROPERTY(getRenderbufferParameter)
	DELETE_FUNCTION_PROPERTY(getShaderParameter)
	DELETE_FUNCTION_PROPERTY(getShaderInfoLog)
	DELETE_FUNCTION_PROPERTY(getShaderPrecisionFormat)
	DELETE_FUNCTION_PROPERTY(getShaderSource)
	DELETE_FUNCTION_PROPERTY(getSupportedExtensions)
	DELETE_FUNCTION_PROPERTY(getTexParameter)
	DELETE_FUNCTION_PROPERTY(getUniform)
	DELETE_FUNCTION_PROPERTY(getUniformLocation)
	DELETE_FUNCTION_PROPERTY(getVertexAttrib)
	DELETE_FUNCTION_PROPERTY(getVertexAttribOffset)
	DELETE_FUNCTION_PROPERTY(hint)
	DELETE_FUNCTION_PROPERTY(isBuffer)
	DELETE_FUNCTION_PROPERTY(isContextLost)
	DELETE_FUNCTION_PROPERTY(isEnabled)
	DELETE_FUNCTION_PROPERTY(isFramebuffer)
	DELETE_FUNCTION_PROPERTY(isProgram)
	DELETE_FUNCTION_PROPERTY(isRenderbuffer)
	DELETE_FUNCTION_PROPERTY(isShader)
	DELETE_FUNCTION_PROPERTY(isTexture)
	DELETE_FUNCTION_PROPERTY(lineWidth)
	DELETE_FUNCTION_PROPERTY(linkProgram)
	DELETE_FUNCTION_PROPERTY(makeXRCompatible)
	DELETE_FUNCTION_PROPERTY(pixelStorei)
	DELETE_FUNCTION_PROPERTY(polygonOffset)
	DELETE_FUNCTION_PROPERTY(readPixels)
	DELETE_FUNCTION_PROPERTY(releaseShaderCompiler)
	DELETE_FUNCTION_PROPERTY(renderbufferStorage)
	DELETE_FUNCTION_PROPERTY(sampleCoverage)
	DELETE_FUNCTION_PROPERTY(scissor)
	DELETE_FUNCTION_PROPERTY(shaderBinary)
	DELETE_FUNCTION_PROPERTY(shaderSource)
	DELETE_FUNCTION_PROPERTY(stencilFunc)
	DELETE_FUNCTION_PROPERTY(stencilFuncSeparate)
	DELETE_FUNCTION_PROPERTY(stencilMask)
	DELETE_FUNCTION_PROPERTY(stencilMaskSeparate)
	DELETE_FUNCTION_PROPERTY(stencilOp)
	DELETE_FUNCTION_PROPERTY(stencilOpSeparate)
	DELETE_FUNCTION_PROPERTY(texImage2D)
	DELETE_FUNCTION_PROPERTY(texParameterf)
	DELETE_FUNCTION_PROPERTY(texParameteri)
	DELETE_FUNCTION_PROPERTY(texSubImage2D)
	DELETE_FUNCTION_PROPERTY(uniform1f)
	DELETE_FUNCTION_PROPERTY(uniform1fv)
	DELETE_FUNCTION_PROPERTY(uniform1i)
	DELETE_FUNCTION_PROPERTY(uniform1iv)
	DELETE_FUNCTION_PROPERTY(uniform2f)
	DELETE_FUNCTION_PROPERTY(uniform2fv)
	DELETE_FUNCTION_PROPERTY(uniform2i)
	DELETE_FUNCTION_PROPERTY(uniform2iv)
	DELETE_FUNCTION_PROPERTY(uniform3f)
	DELETE_FUNCTION_PROPERTY(uniform3fv)
	DELETE_FUNCTION_PROPERTY(uniform3i)
	DELETE_FUNCTION_PROPERTY(uniform3iv)
	DELETE_FUNCTION_PROPERTY(uniform4f)
	DELETE_FUNCTION_PROPERTY(uniform4fv)
	DELETE_FUNCTION_PROPERTY(uniform4i)
	DELETE_FUNCTION_PROPERTY(uniform4iv)
	DELETE_FUNCTION_PROPERTY(uniformMatrix2fv)
	DELETE_FUNCTION_PROPERTY(uniformMatrix3fv)
	DELETE_FUNCTION_PROPERTY(uniformMatrix4fv)
	DELETE_FUNCTION_PROPERTY(useProgram)
	DELETE_FUNCTION_PROPERTY(validateProgram)
	DELETE_FUNCTION_PROPERTY(vertexAttrib1f)
	DELETE_FUNCTION_PROPERTY(vertexAttrib1fv)
	DELETE_FUNCTION_PROPERTY(vertexAttrib2f)
	DELETE_FUNCTION_PROPERTY(vertexAttrib2fv)
	DELETE_FUNCTION_PROPERTY(vertexAttrib3f)
	DELETE_FUNCTION_PROPERTY(vertexAttrib3fv)
	DELETE_FUNCTION_PROPERTY(vertexAttrib4f)
	DELETE_FUNCTION_PROPERTY(vertexAttrib4fv)
	DELETE_FUNCTION_PROPERTY(vertexAttribPointer)
	DELETE_FUNCTION_PROPERTY(viewport)
	DELETE_FUNCTION_PROPERTY(createContext)
	DELETE_FUNCTION_PROPERTY(commit)
}