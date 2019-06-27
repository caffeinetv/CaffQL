#!/bin/sh

find src tests/src -iname "*.hpp" -o -iname "*.cpp" \
| xargs clang-format -i
