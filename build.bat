@echo off

pushd build
rem call emcc ../main.cpp -O2 -s WASM=1 -o main.js -s NO_EXIT_RUNTIME=1 -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'getValue']"
rem call emcc ../main.cpp -O2 -s WASM=1 -o main.js -s NO_EXIT_RUNTIME=1 -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'getValue']"

set Flags=-FC -Zi -nologo -wd4700 -wd4312 -wd4311 -wd4530 -GS-
cl /Fewin_bactorial  ..\win_bactorial.cpp %Flags% -Od /link -incremental:no user32.lib Gdi32.lib kernel32.lib d2d1.lib
popd