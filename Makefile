CC = clang
CXX = clang++
AR = ar

TARGET_EXEC = main

BUILD_DIR = build
MAIN_BUILD = $(BUILD_DIR)/target
IMGUI_BUILD = $(BUILD_DIR)/imgui

SRC_DIRS = ./src
IMGUI_SRC_DIR = ./includes/imgui

WINDOWING = glfw
GRAPHICS = vulkan

SRCS_FULL = $(shell find $(SRC_DIRS) -name '*.cpp')
SRCS = $(SRCS_FULL:$(SRC_DIRS)/%=%)
OBJS = $(SRCS:%=$(MAIN_BUILD)/%.o)
DEPS = $(OBJS:.o=.d)

IMGUI_SRCS = $(shell find $(IMGUI_SRC_DIR) -maxdepth 1 -name '*.cpp') $(IMGUI_SRC_DIR)/misc/cpp/imgui_stdlib.cpp \
			 $(shell find $(IMGUI_SRC_DIR)/backends -name '*$(WINDOWING)*.cpp' -o -name '*$(GRAPHICS)*.cpp')
IMGUI_OBJS = $(IMGUI_SRCS:%=$(IMGUI_BUILD)/%.o)
IMGUI_DEPS = $(IMGUI_OBJS:.o=.d)

INC_DIRS = $(shell find $(SRC_DIRS) -type d)
INC_FLAGS = $(addprefix -I,$(INC_DIRS))

SHADER_SRCS_FULL = $(shell find $(SRC_DIRS) -name '*.vert' -o -name '*.frag')
SHADER_SRCS = $(SHADER_SRCS_FULL:$(SRC_DIRS)/%=%)
SHADER_OBJS = $(SHADER_SRCS:%=$(MAIN_BUILD)/%.spv)

CFLAGS = -Wall -Wformat -I./includes -I./includes/imgui -I./src
IMGUI_CFLAGS = -g -O3 -Wall -Wformat -I$(IMGUI_SRC_DIR) -I$(IMGUI_SRC_DIR)/backends -I$(IMGUI_SRC_DIR)/misc/cpp
CXXFLAGS = -std=c++17
CPPFLAGS = $(INC_FLAGS) -MMD -MP
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lassimp

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS) $(IMGUI_OBJS) $(SHADER_OBJS)
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(OBJS) $(IMGUI_OBJS) -o $(BUILD_DIR)/$(TARGET_EXEC) $(LDFLAGS)

$(MAIN_BUILD)/%.spv: $(SRC_DIRS)/%
	mkdir -p $(dir $@)
	glslc $< -o $@

$(MAIN_BUILD)/%.cpp.o: $(SRC_DIRS)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(IMGUI_BUILD)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(IMGUI_CFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY: test clean release build_debug build_release

clean:
	rm -rf $(BUILD_DIR)

test: clean build_debug
	$(BUILD_DIR)/$(TARGET_EXEC)

build_debug: CFLAGS += -g $(NVIDIA) $(PROTABILITY)
build_debug: $(OBJS) $(SHADER_OBJS) $(BUILD_DIR)/$(TARGET_EXEC)

release: clean build_release
	$(BUILD_DIR)/$(TARGET_EXEC)

build_release: CFLAGS += -DNDEBUG -O3 $(NVIDIA) $(PORTABILITY)
build_release: clean $(BUILD_DIR)/$(TARGET_EXEC)

-include $(DEPS)
-include $(IMGUI_DEPS)
