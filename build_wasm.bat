@echo off

pushd build
set Exported="['_BactorialUpdateWorld', '_BactorialInitWorld']"
call emcc -std=c++11 -Wno-null-dereference ../wasm_bactorial.cpp -O2 -s WASM=1 -o main.js -s NO_EXIT_RUNTIME=1 -s EXPORTED_FUNCTIONS=%Exported% -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'getValue']"
popd