/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * \file getprogramresourceindex.c
 *
 * Tests the errors reported by the GetProgramResourceIndex interface while also
 * testing tricky naming cases. The real functional test is in resource-query.c.
 *
 * From the GL_ARB_program_interface_query spec:
 *      "The command
 *
 *      uint GetProgramResourceIndex(uint program, enum programInterface,
 *                                   const char *name);
 *
 *      returns the unsigned integer index assigned to a resource named <name>
 *      in the interface type <programInterface> of program object <program>.
 *      The error INVALID_ENUM is generated if <programInterface> is
 *      ATOMIC_COUNTER_BUFFER, since active atomic counter buffer resources are
 *      not assigned name strings.
 *
 *      If <name> exactly matches the name string of one of the active resources
 *      for <programInterface>, the index of the matched resource is returned.
 *      Additionally, if <name> would exactly match the name string of an active
 *      resource if "[0]" were appended to <name>, the index of the matched
 *      resource is returned.  Otherwise, <name> is considered not to be the
 *      name of an active resource, and INVALID_INDEX is returned.  Note that if
 *      an interface enumerates a single active resource list entry for an array
 *      variable (e.g., "a[0]"), a <name> identifying any array element other
 *      than the first (e.g., "a[1]") is not considered to match.
 *
 *      For the interface TRANSFORM_FEEDBACK_VARYING, the value INVALID_INDEX
 *      should be returned when querying the index assigned to the special names
 *      "gl_NextBuffer", "gl_SkipComponents1", "gl_SkipComponents2",
 *      "gl_SkipComponents3", and "gl_SkipComponents4".
 *
 *      [...]
 *
 *      An INVALID_VALUE error is generated by GetProgramInterfaceiv,
 *      GetProgramResourceIndex, GetProgramResourceName, GetProgramResourceiv,
 *      GetProgramResourceLocation, and GetProgramResourceLocationIndex if
 *      <program> is not the name of either a shader or program object.
 *
 *      An INVALID_OPERATION error is generated by GetProgramInterfaceiv,
 *      GetProgramResourceIndex, GetProgramResourceName, GetProgramResourceiv,
 *      GetProgramResourceLocation, and GetProgramResourceLocationIndex if
 *      <program> is the name of a shader object.
 *
 *      [...]
 *
 *      INVALID_ENUM is generated by GetProgramResourceIndex if
 *      <programInterface> is ATOMIC_COUNTER_BUFFER."
 */

#include "piglit-util-gl.h"
#include "common.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_core_version = 32;

PIGLIT_GL_TEST_CONFIG_END

/* Naming conventions, from the GL_ARB_program_interface_query extension:
 *
 * "When building a list of active variable or interface blocks, resources
 * with aggregate types (such as arrays or structures) may produce multiple
 * entries in the active resource list for the corresponding interface.
 * Additionally, each active variable, interface block, or subroutine in the
 * list is assigned an associated name string that can be used by
 * applications to refer to the resources.  For interfaces involving
 * variables, interface blocks, or subroutines, the entries of active
 * resource lists are generated as follows:
 *
 *  * For an active variable declared as a single instance of a basic type,
 *    a single entry will be generated, using the variable name from the
 *    shader source.
 *
 *  * For an active variable declared as an array of basic types, a single
 *    entry will be generated, with its name string formed by concatenating
 *    the name of the array and the string "[0]".
 *
 *  * For an active variable declared as a structure, a separate entry will
 *    be generated for each active structure member.  The name of each entry
 *    is formed by concatenating the name of the structure, the "."
 *    character, and the name of the structure member.  If a structure
 *    member to enumerate is itself a structure or array, these enumeration
 *    rules are applied recursively.
 *
 *  * For an active variable declared as an array of an aggregate data type
 *    (structures or arrays), a separate entry will be generated for each
 *    active array element, unless noted immediately below.  The name of
 *    each entry is formed by concatenating the name of the array, the "["
 *    character, an integer identifying the element number, and the "]"
 *    character.  These enumeration rules are applied recursively, treating
 *    each enumerated array element as a separate active variable.
 *
 *  * For an active shader storage block member declared as an array, an
 *    entry will be generated only for the first array element, regardless
 *    of its type.  For arrays of aggregate types, the enumeration rules are
 *    applied recursively for the single enumerated array element.
 *
 *  * For an active interface block not declared as an array of block
 *    instances, a single entry will be generated, using the block name from
 *    the shader source.
 *
 *  * For an active interface block declared as an array of instances,
 *    separate entries will be generated for each active instance.  The name
 *    of the instance is formed by concatenating the block name, the "["
 *    character, an integer identifying the instance number, and the "]"
 *    character.
 *
 *  * For an active subroutine, a single entry will be generated, using the
 *    subroutine name from the shader source.
 *
 * When an integer array element or block instance number is part of the name
 * string, it will be specified in decimal form without a "+" or "-" sign or
 * any extra leading zeroes.  Additionally, the name string will not include
 * white space anywhere in the string.
 */

struct subtest_index_t {
	const char *vs_text;

	GLenum programInterface;
	const char *name;
	bool valid_index;
	GLint expect_value; /* -1, means don't check for an epected value */
	GLenum expected_error;

	const char *programInterface_str;
	const char *error_str;
};

#define ST(vs_text, programInterface, name, valid, value, error) { \
	(vs_text), (programInterface), (name), (valid), (value), (error), \
	#programInterface, #error \
}

/* Test for arrays of arrays */
static const struct subtest_index_t index_subtests[] = {
 ST( vs_empty,      GL_ATOMIC_COUNTER_BUFFER,              "dummy", false, -1, GL_INVALID_ENUM),
 ST( vs_empty,                    GL_UNIFORM,                 NULL, false, -1, GL_NO_ERROR),
 ST( vs_empty,                    GL_UNIFORM,              "dummy", false, -1, GL_NO_ERROR),
 ST( vs_empty,                       GL_TRUE,           "vs_input",  true, -1, GL_INVALID_ENUM),
 ST( vs_array,              GL_PROGRAM_INPUT,           "vs_input",  true, -1, GL_NO_ERROR),
 ST( vs_array,              GL_PROGRAM_INPUT,        "vs_input[0]",  true, -1, GL_NO_ERROR),
 ST( vs_array,              GL_PROGRAM_INPUT,        "vs_input[1]", false, -1, GL_NO_ERROR),
 ST( vs_array,                    GL_UNIFORM,              "hello", false, -1, GL_NO_ERROR),
 ST( vs_array,                    GL_UNIFORM,        "sa[0].hello",  true, -1, GL_NO_ERROR),
 ST( vs_array,                    GL_UNIFORM,        "sa[0].world",  true, -1, GL_NO_ERROR),
 ST( vs_array,                    GL_UNIFORM,     "sa[0].world[0]",  true, -1, GL_NO_ERROR),
 ST( vs_array,                    GL_UNIFORM,        "sa[1].hello", false, -1, GL_NO_ERROR),
 ST(  vs_aofa,              GL_PROGRAM_INPUT,          "vs_input2", false, -1, GL_NO_ERROR),
 ST(  vs_aofa,              GL_PROGRAM_INPUT,       "vs_input2[0]",  true, -1, GL_NO_ERROR),
 ST(  vs_aofa,              GL_PROGRAM_INPUT,    "vs_input2[0][0]",  true, -1, GL_NO_ERROR),
 ST(  vs_aofa,              GL_PROGRAM_INPUT,    "vs_input2[1][0]", false, -1, GL_NO_ERROR),
 ST(  vs_aofa,              GL_PROGRAM_INPUT,    "vs_input2[0][1]", false, -1, GL_NO_ERROR),
 ST(   vs_sub,          GL_VERTEX_SUBROUTINE,                "vss",  true, -1, GL_NO_ERROR),
 ST(   vs_sub,          GL_VERTEX_SUBROUTINE,               "vss2",  true, -1, GL_NO_ERROR),
 ST(vs_subidx,          GL_VERTEX_SUBROUTINE,            "vss_idx",  true,  5, GL_NO_ERROR),
 ST(vs_subidx,          GL_VERTEX_SUBROUTINE,           "vss2_idx",  true, -1, GL_NO_ERROR),
 ST( vs_empty, GL_TRANSFORM_FEEDBACK_VARYING,      "gl_NextBuffer", false, -1, GL_NO_ERROR),
 ST( vs_empty, GL_TRANSFORM_FEEDBACK_VARYING, "gl_SkipComponents1", false, -1, GL_NO_ERROR),
 ST( vs_empty, GL_TRANSFORM_FEEDBACK_VARYING, "gl_SkipComponents2", false, -1, GL_NO_ERROR),
 ST( vs_empty, GL_TRANSFORM_FEEDBACK_VARYING, "gl_SkipComponents3", false, -1, GL_NO_ERROR),
 ST( vs_empty, GL_TRANSFORM_FEEDBACK_VARYING, "gl_SkipComponents4", false, -1, GL_NO_ERROR),
};

static bool
check_extensions(const struct subtest_index_t st)
{
	if (st.programInterface == GL_ATOMIC_COUNTER_BUFFER &&
	    !piglit_is_extension_supported("GL_ARB_shader_atomic_counters")) {
		return false;
	}

	if ((st.programInterface == GL_VERTEX_SUBROUTINE ||
	     st.programInterface == GL_GEOMETRY_SUBROUTINE ||
	     st.programInterface == GL_FRAGMENT_SUBROUTINE ||
	     st.programInterface == GL_COMPUTE_SUBROUTINE ||
	     st.programInterface == GL_VERTEX_SUBROUTINE_UNIFORM ||
	     st.programInterface == GL_GEOMETRY_SUBROUTINE_UNIFORM ||
	     st.programInterface == GL_FRAGMENT_SUBROUTINE_UNIFORM ||
	     st.programInterface == GL_COMPUTE_SUBROUTINE_UNIFORM ||
	     st.programInterface == GL_TESS_CONTROL_SUBROUTINE ||
	     st.programInterface == GL_TESS_EVALUATION_SUBROUTINE ||
	     st.programInterface == GL_TESS_CONTROL_SUBROUTINE_UNIFORM ||
	     st.programInterface == GL_TESS_EVALUATION_SUBROUTINE_UNIFORM ||
	     st.programInterface == GL_COMPUTE_SUBROUTINE_UNIFORM) &&
	     !piglit_is_extension_supported("GL_ARB_shader_subroutine")) {
		 return false;
	 }

	if (st.vs_text == vs_aofa &&
	    !piglit_is_extension_supported("GL_ARB_arrays_of_arrays")) {
		return false;
	}

	return true;
}

static bool
consistency_check(GLuint prog, const struct subtest_index_t st, GLint index)
{
	const GLchar *names[] = { st.name };
	GLuint old_index = 0xdeadcafe;

	/* Validate result against old API. */
	switch (st.programInterface) {
	case GL_UNIFORM:
		glGetUniformIndices(prog, 1, names, &old_index);
		piglit_check_gl_error(GL_NO_ERROR);
		break;
	case GL_UNIFORM_BLOCK:
		old_index = glGetUniformBlockIndex(prog, st.name);
		piglit_check_gl_error(GL_NO_ERROR);
		break;
	case GL_VERTEX_SUBROUTINE:
		old_index = glGetSubroutineIndex(prog, GL_VERTEX_SHADER,
						 st.name);
		piglit_check_gl_error(GL_NO_ERROR);
		break;
	default:
		/* There are no old APIs for this program interface */
		return true;
	}

	if (index != old_index) {
		printf("Index inconsistent with the old API for %s: %i vs %i\n",
		       st.name, index, old_index);
		return false;
	} else
		return true;
}

static void
run_index_subtest(const struct subtest_index_t st, bool *pass)
{
	enum piglit_result result;
	bool local_pass = true;
	GLint index;
	GLuint prog;

	if (!check_extensions(st)) {
		result = PIGLIT_SKIP;
		goto report_result;
	}

	prog = piglit_build_simple_program(st.vs_text, NULL);
	if (!piglit_link_check_status(prog)) {
		glDeleteProgram(prog);
		result = PIGLIT_FAIL;
		*pass = false;
		goto report_result;
	}

	index = glGetProgramResourceIndex(prog, st.programInterface, st.name);
	if (!piglit_check_gl_error(st.expected_error)) {
		printf("Call was glGetProgramResourceIndex(prog, %s, "
		       "%s, ...) = %i\n", st.programInterface_str, st.name,
		       index);
		local_pass = false;
	} else if (st.expected_error == GL_NO_ERROR) {
                if (index >=0 && !st.valid_index) {
			printf("Invalid index for '%s': expected INVALID_INDEX "
			       "but got %i\n", st.name, index);
			local_pass = false;
                } else if (index >=0 && st.expect_value != -1 &&
                           st.expect_value != index) {
			printf("For index of '%s': expected %d "
			       "but got %i\n", st.name, st.expect_value, index);
			local_pass = false;
		} else if (index == GL_INVALID_INDEX && st.valid_index) {
			printf("Invalid index for '%s': expected a valid index "
			       "but got INVALID_INDEX\n", st.name);
			local_pass = false;
		} else if (!consistency_check(prog, st, index)) {
			local_pass = false;
		}
	}

	glDeleteProgram(prog);

	*pass = *pass && local_pass;
	result = local_pass ? PIGLIT_PASS : PIGLIT_FAIL;

report_result:
	piglit_report_subtest_result(result, "'%s' on %s", st.name,
				     st.programInterface_str);
}

void
piglit_init(int argc, char **argv)
{
	piglit_require_extension("GL_ARB_program_interface_query");
}

enum piglit_result
piglit_display(void)
{
	bool pass = true, prg_tst;
	GLuint shader, test_count;
	int i;

	/* test using an unexisting program ID */
	glGetProgramResourceIndex(1337, GL_UNIFORM, "resource");
	prg_tst = piglit_check_gl_error(GL_INVALID_VALUE);
	pass = pass && prg_tst;
	piglit_report_subtest_result(prg_tst ? PIGLIT_PASS : PIGLIT_FAIL,
				     "Invalid program (undefined ID)");

	/* test using a shader ID */
	shader = piglit_compile_shader_text(GL_VERTEX_SHADER, vs_empty);
	glGetProgramResourceIndex(shader, GL_UNIFORM, "resource");
	prg_tst = piglit_check_gl_error(GL_INVALID_OPERATION);
	pass = pass && prg_tst;
	piglit_report_subtest_result(prg_tst ? PIGLIT_PASS : PIGLIT_FAIL,
				     "Invalid program (call on shader)");

	/* run all the getprograminterfaceiv tests */
	test_count = sizeof(index_subtests) / sizeof(struct subtest_index_t);
	for (i = 0; i < test_count; i++) {
		run_index_subtest(index_subtests[i], &pass);
	}

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}
