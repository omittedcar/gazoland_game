default := gles vulkan

vulkan : bin/gazoland-vulkan

gles : bin/gazoland-gles

CC := clang
CXX := clang++
CFLAGS :=
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
	platform
GLES_SOURCES := $(SOURCES) gazoland_gles
VULKAN_SOURCES := $(SOURCES) gazoland_vulkan

bin obj :
	mkdir $@

bin/gazoland-gles : $(GLES_SOURCES:%=obj/%.o) | bin
	$(CXX) $(LDFLAGS) $(LIBS) -o $@ $^

bin/gazoland-vulkan : $(VULKAN_SOURCES:%=obj/%.o) | bin
	$(CXX) $(LDFLAGS) $(LIBS) -o $@ $^

obj/%.o : src/%.cpp src/*.h | obj
	$(CXX) $(CFLAGS) -c -o $@ $<

obj/%.o : src/%.c src/*.h | obj
	$(CC) -std=c23 $(CFLAGS) -c -o $@ $<

clean :
	rm -rf bin obj
