#pragma once

#include "jsonst.h"

const char *jsonst_type_to_str(jsonst_type type);

void jsonst_path_print(const jsonst_path *path);

void jsonst_path_print_reverse(const jsonst_path *path);
