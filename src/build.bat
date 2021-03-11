@echo off

set compiler=gcc -g -DBUILD_DEBUG -DOS_WINDOWS
REM set compiler=gcc -O3 -DBUILD_DEBUG -DOS_WINDOWS
set linker=-lm -lgdi32 -lopengl32

%compiler% main.c -o ../run/editor_win64.exe %linker%
echo "Editor build successfully"
