# PV112 2017, lesson 3

binary=cv3
files=$(binary)_main.cpp PV112.cpp
CXX ?= g++
CXXFLAGS := -std=c++11 -lGL -lglut -lGLEW $(CXXFLAGS)

# Use custom libraries (see 'LIBRARIES' below)
#mylibs = ../../libs
#export LD_LIBRARY_PATH = $(mylibs)
#export CXXFLAGS := -I $(mylibs) -L $(mylibs) $(CXXFLAGS)

$(binary): $(files) PV112.h
	$(CXX) -o $(binary) $(files) $(CXXFLAGS)

run: $(binary)
	./$(binary)

clean:
	env
	rm $(binary)

# LIBRARIES:
# You need to have development version of the libraries above installed.
# On Debian (and thus probably Ubuntu), those are in packages:
# libgl-dev, freeglut3-dev, libglew-dev, libglm-dev; later also libdevil-dev.
# If you cannot install development libraries (e.g. in the PC room), you can
# download and unpack linux_libs.zip from the course's study materials and then
# follow instructions there on how to symlink to existing non-development
# libraries. However, you might (and on school Linux computers, you will, but I
# can help you) run into issues with version mismatch, missing libraries etc.
# You can also try static linking for some of the libraries. If you have any
# problems with compiling and linking on Linux, do not hesitate to ask for my
# help, preferably BEFORE your lesson. You can reach me at
# <adamat@mail.muni.cz>.

