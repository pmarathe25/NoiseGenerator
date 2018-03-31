include ../makefile.defines
BUILDDIR = build
BINDIR = ~/bin
TESTDIR = test
SRCDIR = src
INCLUDEDIR = include
# Objects
TESTOBJS = $(call generate_obj_names,$(TESTDIR),$(BUILDDIR))
# Headers
INCLUDE = -I$(INCLUDEDIR)
HEADERS = $(call find_headers,$(INCLUDEDIR))
# Compiler settings
CXX = g++
CFLAGS = -fPIC -c -std=c++17 $(INCLUDE) -flto -O3 -Wpedantic -march=native
LFLAGS = -shared -flto -O3 -march=native
TESTLFLAGS = -lstealthcolor -lsfml-graphics -lsfml-window -lsfml-system -flto -O3 -march=native

.PHONY: clean lib install uninstall

$(TESTDIR)/noiseTest: $(BUILDDIR)/noiseTest.o $(HEADERS) $(OBJS)
	$(CXX) $(BUILDDIR)/noiseTest.o $(OBJS) $(TESTLFLAGS) -o $(TESTDIR)/noiseTest

$(BUILDDIR)/noiseTest.o: $(TESTDIR)/noiseTest.cpp $(HEADERS)
	$(CXX) $(CFLAGS) $(TESTDIR)/noiseTest.cpp -o $(BUILDDIR)/noiseTest.o

clean:
	rm $(OBJS) $(TESTOBJS) $(TESTDIR)/noiseTest

test: $(TESTDIR)/noiseTest
	$(TESTDIR)/noiseTest

install:
	$(call install_headers,$(INCLUDEDIR),NoiseGenerator,$(HEADERS))

uninstall:
	$(call uninstall_headers,NoiseGenerator)
