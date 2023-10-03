CC = clang
CXX = clang++

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
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS) $(SHADER_OBJS)
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(OBJS) -o $(BUILD_DIR)/$(TARGET_EXEC) $(LDFLAGS)

#$(MAIN_BUILD)/%.c.o: %.c
#	mkdir -p $(dir $@)
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(MAIN_BUILD)/%.spv: $(SRC_DIRS)/%
	mkdir -p $(dir $@)
	glslc $< -o $@

$(MAIN_BUILD)/%.cpp.o: $(SRC_DIRS)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(IMGUI_BUILD)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(IMGUI_CFLAGS) -c $< -o $@

.PHONY: test clean release build_debug build_release

test: CFLAGS += -g
test: clean $(BUILD_DIR)/$(TARGET_EXEC)
	$(BUILD_DIR)/$(TARGET_EXEC)

clean:
	rm -rf $(BUILD_DIR)

release: CFLAGS += -DNDEBUG -O3
release: clean test

build_debug: CFLAGS += -g
build_debug: clean $(BUILD_DIR)/$(TARGET_EXEC)

build_release: CFLAGS += -DNDEBUG -O3
build_release: clean $(BUILD_DIR)/$(TARGET_EXEC)

-include $(DEPS)
-include $(IMGUI_DEPS)
