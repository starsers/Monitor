#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need GLFW (http://www.glfw.org):
# You will also need OpenCV
# Linux:
#   apt-get install libglfw3-dev
#   apt-get install libopencv-dev
# Mac OS X:
#   brew install glfw
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
#

CXX = g++
#CXX = clang++

EXE = main
IMGUI_DIR = imgui-docking

SOURCES = src/app/main.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp


SRC_DIR = src
INCLUDE_DIR = include
MODELS = app monitor
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp) $(foreach model, $(MODELS), $(wildcard $(SRC_DIR)/$(model)/*.cpp))

# 平台检测
UNAME_S := $(shell uname -s)

# 移除特定于平台的文件，以避免冲突
ifeq ($(UNAME_S), Linux)
    # 在Linux上，排除Windows特定文件
    SRC_FILES := $(filter-out $(SRC_DIR)/monitor/WindowCam.cpp, $(SRC_FILES))
    # 添加平台定义
    CXXFLAGS += -D__linux__
else
    # 在Windows上，排除Linux特定文件
    SRC_FILES := $(filter-out $(SRC_DIR)/monitor/LinuxCam.cpp, $(SRC_FILES))
    # 添加Windows平台定义
    ifeq ($(OS), Windows_NT)
        CXXFLAGS += -D_WIN32
    endif
endif

OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, %.o, $(SRC_FILES))

SOURCES += $(SRC_FILES)

OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

LINUX_GL_LIBS = -lGL

CXXFLAGS = -std=c++17 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -g -Wall -Wformat
LIBS =
CXXFLAGS += $(foreach model,$(MODELS), -I$(INCLUDE_DIR)/$(model))
CXXFLAGS += $(shell pkg-config --cflags opencv4)
CXXFLAGS += $(shell pkg-config --cflags glfw3)
LIBS     += $(shell pkg-config --libs opencv4)
LIBS     += $(shell pkg-config --libs glfw3)


##---------------------------------------------------------------------
## OPENGL ES
##---------------------------------------------------------------------

## This assumes a GL ES library available in the system, e.g. libGLESv2.so
# CXXFLAGS += -DIMGUI_IMPL_OPENGL_ES2
# LINUX_GL_LIBS = -lGLESv2

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += $(LINUX_GL_LIBS) `pkg-config --static --libs glfw3`

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib -L/opt/local/lib -L/opt/homebrew/lib
	#LIBS += -lglfw3
	LIBS += -lglfw

	CXXFLAGS += -I/usr/local/include -I/opt/local/include -I/opt/homebrew/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lglfw3 -lgdi32 -lopengl32 -limm32

	CXXFLAGS += $(shell pkg-config --cflags glfw3)
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(SRC_DIR)/app/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(SRC_DIR)/monitor/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
