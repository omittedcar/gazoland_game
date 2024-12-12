# Modify for your local system

LLVM := /usr/lib/llvm-19
CC := $(LLVM)/bin/clang
CXX := $(LLVM)/bin/clang++
CPPFLAGS := -I/usr/include/c++/13 -I/usr/include/x86_64-linux-gnu/c++/13
CFLAGS := -std=c23
LDFLAGS := -L/lib/x86_64-linux-gnu

# Do not modify below this line

DEBUGFLAGS := -O0 -g
OBJECTS := resources.o png_decoder.o game.o gazo.o main.o shader.o
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
