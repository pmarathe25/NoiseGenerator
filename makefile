BUILDDIR = build/
BINDIR = ~/bin/
TESTDIR = test/
SRCDIR = src/
# Objects
TESTOBJS = $(addprefix $(BUILDDIR)/, noiseTest.o)
# Headers
INCLUDEPATH = include/
INCLUDE = -I$(INCLUDEPATH)
HEADERS = $(addprefix $(INCLUDEPATH)/, InternalCaches.hpp NoiseGenerator.hpp NoiseGeneratorUtil.hpp NoiseGenerator1D.hpp NoiseGenerator2D.hpp NoiseGenerator3D.hpp)
# Compiler settings
CXX = g++
CFLAGS = -fPIC -c -std=c++17 $(INCLUDE) -O3 -Wpedantic -march=native -flto
LFLAGS = -shared -flto -march=native
TESTLFLAGS = -lstealthcolor -lsfml-graphics -lsfml-window -lsfml-system -pthread -flto -march=native

$(TESTDIR)/noiseTest: $(BUILDDIR)/noiseTest.o $(HEADERS) $(OBJS)
	$(CXX) $(BUILDDIR)/noiseTest.o $(OBJS) $(TESTLFLAGS) -o $(TESTDIR)/noiseTest

$(BUILDDIR)/noiseTest.o: $(TESTDIR)/noiseTest.cpp $(HEADERS)
	$(CXX) $(CFLAGS) $(TESTDIR)/noiseTest.cpp -o $(BUILDDIR)/noiseTest.o

clean:
	rm $(OBJS) $(TESTOBJS) $(TESTDIR)/noiseTest

test: $(TESTDIR)/noiseTest
	$(TESTDIR)/noiseTest
