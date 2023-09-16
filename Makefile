CC = clang
CXX = clang++

TARGET_EXEC = main

BUILD_DIR = build
MAIN_BUILD = $(BUILD_DIR)/target
IMGUI_BUILD = $(BUILD_DIR)/imgui

SRC_DIRS = ./src ./includes/src
IMGUI_SRC_DIR = ./includes/imgui

WINDOWING = glfw
GRAPHICS = vulkan

SRCS = $(shell find $(SRC_DIRS) -name '*.cpp')
OBJS = $(SRCS:%=$(MAIN_BUILD)/%.o)
DEPS = $(OBJS:.o=.d)

IMGUI_SRCS = $(shell find $(IMGUI_SRC_DIR) -maxdepth 1 -name '*.cpp') $(IMGUI_SRC_DIR)/misc/cpp/imgui_stdlib.cpp \
			 $(shell find $(IMGUI_SRC_DIR)/backends -name '*$(WINDOWING)*.cpp' -o -name '*$(GRAPHICS)*.cpp')
IMGUI_OBJS = $(IMGUI_SRCS:%=$(IMGUI_BUILD)/%.o)
IMGUI_DEPS = $(IMGUI_OBJS:.o=.d)

INC_DIRS = $(shell find $(SRC_DIRS) -type d)
INC_FLAGS = $(addprefix -I,$(INC_DIRS))

CFLAGS = -g -O3 -Wall -Wformat -I./includes -I./includes/imgui
IMGUI_CFLAGS = -g -O3 -Wall -Wformat -I$(IMGUI_SRC_DIR) -I$(IMGUI_SRC_DIR)/backends -I$(IMGUI_SRC_DIR)/misc/cpp
CXXFLAGS = -std=c++17
CPPFLAGS = $(INC_FLAGS) -MMD -MP
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS) $(IMGUI_OBJS)
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(OBJS) $(IMGUI_OBJS) -o $(BUILD_DIR)/$(TARGET_EXEC) $(LDFLAGS)

#$(MAIN_BUILD)/%.c.o: %.c
#	mkdir -p $(dir $@)
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(MAIN_BUILD)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(IMGUI_BUILD)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(IMGUI_CFLAGS) -c $< -o $@

.PHONY: test clean

test: $(BUILD_DIR)/$(TARGET_EXEC)
	$(BUILD_DIR)/$(TARGET_EXEC)

clean:
	rm -r $(BUILD_DIR)

-include $(DEPS)
-include $(IMGUI_DEPS)
