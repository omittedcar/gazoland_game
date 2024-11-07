# Modify for your local system

CC := clang
CXX := g++
CFLAGS :=
LDFLAGS := -L/lib/x86_64-linux-gnu

# Do not modify below this line

OBJECTS := game.o gazo.o main.o shader.o
LIBS := -lglfw -lGLESv2 -lEGL

default : all

all : bin/gazoland

bin/gazoland : $(OBJECTS:%.o=obj/%.o)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

obj/%.o : src/%.cc
	mkdir -p obj && $(CXX) -c -o $@ $(CFLAGS) $<

clean :
	rm -f obj/*.o bin/gazoland
