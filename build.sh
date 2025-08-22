#!/usr/bin/env bash

log_run() {
    echo "[I] $@"
    $@
}

CFLAGS=(-std=c99 -Og -g3)

log_run \
    cc "${CFLAGS[@]}" build.c "jv_$(uname -m).s"
