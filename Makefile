default := all

all : gles vulkan

vulkan : bin/gazoland-vulkan

gles : bin/gazoland-gles

CC := clang-19
CXX := clang++
CFLAGS := -std=c++2b
ACTUALCFLAGS := -std=c23
vulkan : CFLAGS += -DGAZOLAND_VULKAN
gles : CFLAGS += -DGAZOLAND_GLES

LIBS := -lglfw
vulkan : LIBS += -lvulkan
gles : LIBS += -lGLESv2 -lEGL

SOURCES := \
	game \
	gazo \
	level \
	main \
	path \
	platform \
	embeds
GLES_SOURCES := $(SOURCES:%=%_gl) gazoland_gles
VULKAN_SOURCES := $(SOURCES:%=%_vk) gazoland_vulkan

bin obj :
	mkdir $@

bin/gazoland-gles : $(GLES_SOURCES:%=obj/%.o) | bin
	$(CXX) $(LDFLAGS) $(LIBS) -o $@ $^

bin/gazoland-vulkan : $(VULKAN_SOURCES:%=obj/%.o) | bin
	$(CXX) $(LDFLAGS) $(LIBS) -o $@ $^

obj/%.o : src/%.cpp src/*.h | obj
	$(CXX) $(CFLAGS) -c -o $@ $<

obj/%_gl.o : src/%.c src/*.h | obj
	$(CC) $(ACTUALCFLAGS) -c -o $@ $<

obj/%_gl.o : src/%.cpp src/*.h | obj
	$(CXX) $(CFLAGS) -c -o $@ $<

obj/%_vk.o : src/%.cpp src/*.h | obj
	$(CXX) $(CFLAGS) -c -o $@ $<

obj/%_vk.o : src/%.c src/*.h | obj
	$(CC) $(ACTUALCFLAGS) -c -o $@ $<

clean :
	rm -rf bin obj
		