# Modify for your local system

CC := clang
CXX := clang++
CFLAGS := -I/usr/include/c++/13 -I/usr/include/x86_64-linux-gnu/c++/13
LDFLAGS := -L/lib/x86_64-linux-gnu

# Do not modify below this line

DEBUGFLAGS := -O0 -g
OBJECTS := game.o gazo.o main.o shader.o
LIBS := -lglfw -lGLESv2 -lEGL

default : bin/gazoland

debug : bin/gazoland-dbg

bin/gazoland : $(OBJECTS:%.o=obj/%.o)
	mkdir -p bin && $(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

bin/gazoland-dbg : $(OBJECTS:%.o=obj/%-dbg.o)
	mkdir -p bin && $(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

obj/%.o : src/%.cc
	mkdir -p obj && $(CXX) -c -o $@ $(CFLAGS) $<

obj/%-dbg.o : src/%.cc
	mkdir -p obj && $(CXX) -c -o $@ $(CFLAGS) $(DEBUGFLAGS) $<

clean :
	rm -f obj/*.o bin/gazoland bin/gazoland-dbg
