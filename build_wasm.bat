@echo off

pushd build
set Exported="['_BactorialUpdateWorld', '_BactorialInitWorld', '_BactorialSelect', '_BactorialDivide', '_BactorialUnselect', '_BactorialSpawnEnemy']"
call emcc -DWASM=1 -s TOTAL_MEMORY=268435456 -s ASSERTIONS=1 -std=c++11 -Wno-null-dereference ../wasm_bactorial.cpp -O2 -s WASM=1 -o main.js -s NO_EXIT_RUNTIME=1 -s EXPORTED_FUNCTIONS=%Exported% -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'getValue']"
popd