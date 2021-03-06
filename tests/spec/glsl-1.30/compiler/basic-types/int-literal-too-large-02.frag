// [config]
// expect_result: fail
// glsl_version: 1.30
// [end config]
//
// Unsigned integer literals that are too large should raise an error.
//
// From page 22 (28 of PDF) of GLSL 1.30 spec:
//     "It is an error to provide a literal integer whose magnitude is too
//     large to store in a variable of matching signed or unsigned type."
//
//     Unsigned integers have exactly 32 bits of precision.  Signed integers
//     use 32 bits, including a sign bit, in two's complement form.

#version 130

uint f() {
	// Requires 33 bits.
	return uint(0x1ffffffffu);
}
