OS := $(shell uname)

ifeq ($(OS),Darwin)
CXX = clang++
SDL = -framework SDL2 -framework SDL2_image -framework SDL2_ttf -F/Library/Frameworks/
SDLINC =
else
CXX = g++
SDL = `sdl2-config --cflags --libs` -lSDL2_image
SDLINC = `sdl2-config --cflags --libs`
endif

CXXFLAGS = -Wall -g -c -D_AIX_PTHREADS_D7 -std=c++14 $(SDLINC) -I/Library/Frameworks/SDL2.framework/Headers/ -I/Library/Frameworks/SDL2_image.framework/Headers/ -I/Library/Frameworks/SDL2_ttf.framework/Headers/
LDFLAGS = $(SDL) -lportaudio -lpthread
EXE = bin/vumeter

CPP_FILES := $(wildcard src/*.cpp)
OBJ_FILES := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))

all: $(EXE)

$(EXE): $(OBJ_FILES)
	$(CXX) $(LDFLAGS) $(OBJ_FILES) -o $@

obj/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<


clean:
	rm obj/*.o && rm $(EXE)
