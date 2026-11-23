#!/bin/sh
# ax-gpu.sh — Mali-G31 MP2 (Panfrost Bifrost Gen1) Zero-Stutter 640x480 Environment
mkdir -p /tmp/mesa_shader_cache
export MESA_SHADER_CACHE_DIR=/tmp/mesa_shader_cache
export MESA_SHADER_CACHE_MAX_SIZE=16M
export PAN_MESA_DEBUG=noafbc
export SDL_VIDEODRIVER=kmsdrm
