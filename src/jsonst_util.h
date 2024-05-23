#ifndef JSONST_UTIL_H
#define JSONST_UTIL_H

// jsonst_util.{h,cc}: Optional utilities for jsonst.

#include "jsonst.h"

#define JSONST_UNKNOWN "__UNKNOWN__"

// jsonst_type_to_str() returns a string representation of the given jsonst_type.
// Returns JSONST_UNKNOWN if the type is not known.
const char *jsonst_type_to_str(const jsonst_type type);

// jsonst_error_to_str() returns a string representation of the given jsonst_error.
// Returns JSONST_UNKNOWN if the error is not known.
const char *jsonst_error_to_str(const jsonst_error type);

#endif
