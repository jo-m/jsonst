#!/usr/bin/env bash

shopt -s globstar
clang-format -i src/**/*.{h,c,cc} examples/**/*.c fuzz/**/*.c
