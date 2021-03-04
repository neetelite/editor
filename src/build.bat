@echo off

IF NOT EXIST ..\run mkdir ..\run
pushd ..\run

set compiler= -nologo -Od -Zi -DBUILD_DEBUG -DOS_WINDOWS -wd4477
set linker= -link user32.lib gdi32.lib winmm.lib opengl32.lib
cl %compiler% ..\src\main.c %linker%

popd
