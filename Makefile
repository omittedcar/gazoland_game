
# Modify for your local system

LLVM := /lib/llvm-19

# Do not modify below this line

CC := $(LLVM)/bin/clang
CXX := $(LLVM)/bin/clang++
CFLAGS := -std=c23
DEBUGFLAGS := -O0 -g
OBJECTS := resources.o path.o png_decoder.o main.o game.o level.o platform.o gazo.o gl_program_info.o
LIBS := -lglfw -lGLESv2 -lEGL -lc++

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
