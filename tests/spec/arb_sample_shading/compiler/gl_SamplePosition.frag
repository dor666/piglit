// [config]
// expect_result: pass
// glsl_version: 1.30
// require_extensions: GL_ARB_sample_shading
// [end config]
#version 130
#extension GL_ARB_sample_shading : enable

vec2 func()
{
	return gl_SamplePosition;
}
