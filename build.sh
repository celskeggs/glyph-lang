#!/bin/bash
set -e -u

clang -Wall -Werror glyph-core.c glyph-api.c -ldl -lyaml -Wl,--export-dynamic -o glyph
