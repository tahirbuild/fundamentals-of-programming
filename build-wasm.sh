#!/usr/bin/env bash
# Compiles the C++ core to WebAssembly for the React app.
# Requires the Emscripten SDK on PATH (source emsdk_env.sh first).
# The output is committed to web/src/wasm/, so Vercel does not need Emscripten.
set -euo pipefail
cd "$(dirname "$0")"

mkdir -p web/src/wasm
em++ cpp/grade_core.cpp cpp/wasm_api.cpp \
  -std=c++17 -O2 \
  -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME=createGradeModule \
  -s ENVIRONMENT=web -s SINGLE_FILE=1 -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_RUNTIME_METHODS='["cwrap","UTF8ToString"]' \
  -o web/src/wasm/grade_manager.js

echo "Built web/src/wasm/grade_manager.js"
