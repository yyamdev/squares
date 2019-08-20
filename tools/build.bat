@echo off

set output_file=build\squares.wasm

mkdir build
del %output_file%

echo Building %output_file%

REM   Compile both source files into LLVM bitcode
clang src/squares.c -emit-llvm -c -o llvm_bitfile_squares.bc --target=wasm32
clang src/shared.c -emit-llvm -c -o llvm_bitfile_shared.bc --target=wasm32

REM   Link object files to create WASM module.
REM The module defines the size of its memory. (It exports memory rather than imports it)
REM The memory size must be a multiple of 64k which is one page, and must be at least 2 pages large.
REM This script defines 64k * 64 = 4MB of memory.
set page_size=65536
set /A total_memory_size=%page_size%*64
set /A stack_size=%page_size%

wasm-ld llvm_bitfile_squares.bc llvm_bitfile_shared.bc ^
-O2 -o %output_file% --no-entry ^
--initial-memory=%total_memory_size% ^
--max-memory=%total_memory_size% ^
--stack-first ^
-z stack-size=%stack_size% ^
--export js_on_startup ^
--export js_on_frame ^
--export js_on_keyboard_event ^
--export js_on_image_loaded

echo Done!
echo Copying files...

REM   Copy the web page to build/
xcopy .\src\index.html .\build\index.html /f /y /i /q

REM   Copy the contents of the assets folder to build/assets/
xcopy .\assets .\build\assets /e /y /q

del llvm_bitfile_squares.bc
del llvm_bitfile_shared.bc

echo Done!