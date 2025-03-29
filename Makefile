
# Modify for your local system

LLVM := ~/elelviem/LLVM-20.1.0-Linux-X64
CC := $(LLVM)/bin/clang
CXX := $(LLVM)/bin/clang++
#CPPFLAGS := -I/usr/include/c++/13 -I/usr/include/x86_64-linux-gnu/c++/13
CPPFLAGS := -I/home/glebem/elelviem/LLVM-20.1.0-Linux-X64/include/c++/v1 -I/home/glebem/elelviem/LLVM-20.1.0-Linux-X64/include/x86_64-unknown-linux-gnu/c++/v1
CFLAGS := -std=c23
#LDFLAGS := -L/lib/x86_64-linux-gnu
LDFLAGS := -L/usr/lib/x86_64-linux-gnu -L/home/glebem/elelviem/LLVM-20.1.0-Linux-X64/lib/x86_64-unknown-linux-gnu -L/home/glebem/elelviem/LLVM-20.1.0-Linux-X64/lib/clang/20/lib/x86_64-unknown-linux-gnu
# -fuse-ld=ld.lld
# Do not modify below this line

DEBUGFLAGS := -O0 -g
OBJECTS := resources.o png_decoder.o main.o game.o level.o platform.o gazo.o
LIBS := -lglfw -lGLESv2 -lEGL 

default : bin/gazoland_for_linux.exe

debug : bin/gazoland_for_linux_debug.exe

bin/gazoland_for_linux.exe : $(OBJECTS:%.o=obj/%.o)
	mkdir -p bin && $(CXX) $(LDFLAGS) $(LIBS) -o $@ $^

bin/gazoland_for_linux_debug.exe : $(OBJECTS:%.o=obj/%-dbg.o)
	mkdir -p bin && $(CXX) $(LDFLAGS) $(LIBS) -o $@ $^

obj/%.o : src/%.cc src/*.h
	mkdir -p obj && $(CXX) -c $(CPPFLAGS) -o $@ $<

obj/%.o : src/%.c src/sample_text.txt src/*.h
	mkdir -p obj && $(CC) -c $(CFLAGS) -o $@ $<

obj/%-dbg.o : src/%.cc src/*.h
	mkdir -p obj && $(CXX) -c $(CPPFLAGS) $(DEBUGFLAGS) -o $@ $<

obj/%-dbg.o : src/%.c src/*.h
	mkdir -p obj && $(CC) -c $(CFLAGS) $(DEBUGFLAGS) -o $@ $<

clean :
	rm -f obj/*.o bin/gazoland_for_linux.exe bin/gazoland_for_linux_debug.exe
