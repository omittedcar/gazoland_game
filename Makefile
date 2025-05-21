CC := clang
CXX := clang++
CFLAGS := -std=c23
DEBUGFLAGS := -O0 -g
SOURCES := \
	game \
	gazo \
	gl_program_info \
	level \
	main \
	path \
	platform \
	png_decoder \
	resources

LIBS := -lglfw -lGLESv2 -lEGL -lstdc++

default : bin/gazoland_for_linux.exe

debug : bin/gazoland_for_linux_debug.exe

bin/gazoland_for_linux.exe : $(SOURCES:%=obj/%.o)
	mkdir -p bin && $(CXX) $(LDFLAGS) $(LIBS) -o $@ $^

bin/gazoland_for_linux_debug.exe : $(SOURCES:%=obj/%-dbg.o)
	mkdir -p bin && $(CXX) $(LDFLAGS) $(LIBS) -o $@ $^

obj/%.o : src/%.cpp src/*.h
	mkdir -p obj && $(CXX) -c $(CPPFLAGS) -o $@ $<

obj/%.o : src/%.c src/sample_text.txt src/*.h
	mkdir -p obj && $(CC) -c $(CFLAGS) -o $@ $<

obj/%-dbg.o : src/%.cpp src/*.h
	mkdir -p obj && $(CXX) -c $(CPPFLAGS) $(DEBUGFLAGS) -o $@ $<

obj/%-dbg.o : src/%.c src/*.h
	mkdir -p obj && $(CC) -c $(CFLAGS) $(DEBUGFLAGS) -o $@ $<

clean :
	rm -f obj/*.o bin/gazoland_for_linux.exe bin/gazoland_for_linux_debug.exe
