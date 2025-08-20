#!/usr/bin/env sh

cc -std=c99 -Og -g3 build.c "jv_$(uname -m).s"
