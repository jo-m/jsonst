#!/bin/bash

shopt -s globstar
clang-format --dry-run --Werror src/**/*.{h,c,cc} examples/**/*.c
