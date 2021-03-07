output="editor"

#linux
#compiler="-g -DBUILD_DEBUG -DOS_LINUX -Wall -Wextra"
compiler="-g -DBUILD_DEBUG -DOS_LINUX"
linker="-lm -lX11 -lGL"

#windows
#compiler="-g -DBUILD_DEBUG -DOS_WINDOWS -Wall"
#linker="-lm -lopengl32 -lgdi32"

gcc $compiler main.c -o ../run/$output $linker
