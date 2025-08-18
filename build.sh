#!/usr/bin/env sh

cc -std=c99 -Og -g3 build.c "$(uname -m)_jv.s"
