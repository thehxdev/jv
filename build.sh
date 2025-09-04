#!/usr/bin/env bash

log_run() {
    echo "[I] $@"
    $@
}

ARCH=$(uname -m)
CFLAGS=(-std=c99 -Og -g3)

sed "s/@ARCH@/${ARCH}/g" build.c.in > build.c

log_run \
    cc "${CFLAGS[@]}" build.c arch/$ARCH/*.s
