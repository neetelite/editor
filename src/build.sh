output="editor"

#linux
#compiler="-g -DBUILD_DEBUG -DOS_WINDOWS -Wall -Wextra"
#linker="-lm -lX11 -lGL"

#windows
compiler="-g -DBUILD_DEBUG -DOS_WINDOWS -Wall"
linker="-lm -lopengl32 -lgdi32"

gcc $compiler main.c -o ../run/$output $linker
