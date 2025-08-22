#!/usr/bin/env sh

log_run() {
    echo "[I] $@"
    $@
}

log_run \
    cc -std=c99 -Og -g3 build.c "jv_$(uname -m).s"
