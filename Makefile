default := all

all : gles vulkan

vulkan : bin/gazoland-vulkan spirv

gles : bin/gazoland-gles

CC := clang
CXX := clang++
GLSLC := glslc
CXXFLAGS := -std=c++2b
CFLAGS := -std=c23
vulkan : CXXFLAGS += -DGAZOLAND_VULKAN
gles : CXXFLAGS += -DGAZOLAND_GLES

LIBS := -lglfw
vulkan : LIBS += -lvulkan -lwayland-client
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

FRAG_SHADERS := \
       frag_basic \
       frag_gamma \
       frag_gui
$(FRAG_SHADERS:%=assets/spirv/%.spv) : SHADER_STAGE=fragment

VERT_SHADERS := \
       vert_3d \
       vert_basic \
       vert_gazo \
       vert_no_uv_map
$(VERT_SHADERS:%=assets/spirv/%.spv) : SHADER_STAGE=vertex

spirv : $(FRAG_SHADERS:%=assets/spirv/%.spv) $(VERT_SHADERS:%=assets/spirv/%.spv)

bin obj assets/spirv :
	mkdir $@

bin/gazoland-gles : $(GLES_SOURCES:%=obj/%.o) | bin
	$(CXX) $(LDFLAGS) $(LIBS) -o $@ $^

bin/gazoland-vulkan : $(VULKAN_SOURCES:%=obj/%.o) | bin
	$(CXX) $(LDFLAGS) $(LIBS) -o $@ $^

obj/%.o : src/%.cpp src/*.h | obj
	$(CXX) $(CXXFLAGS) -c -o $@ $<

obj/%_gl.o : src/%.c src/*.h | obj
	$(CC) $(CFLAGS) -c -o $@ $<

obj/%_gl.o : src/%.cpp src/*.h | obj
	$(CXX) $(CXXFLAGS) -c -o $@ $<

obj/%_vk.o : src/%.cpp src/*.h | obj
	$(CXX) $(CXXFLAGS) -c -o $@ $<

obj/%_vk.o : src/%.c src/*.h | obj
	$(CC) $(CFLAGS) -c -o $@ $<

assets/spirv/%.spv : assets/glsl/%.glsl | assets/spirv
	$(GLSLC) -c -fshader-stage=$(SHADER_STAGE) -o $@ $<

clean :
	rm -rf bin obj assets/spirv
