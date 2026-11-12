.DEFAULT_GOAL := release

CXX ?= c++
PYTHON_PREFIX ?= /opt/anaconda3
PYTHON_VERSION ?= 3.11
PYTHON_INCLUDE ?= $(PYTHON_PREFIX)/include/python$(PYTHON_VERSION)
PYTHON_LIBDIR ?= $(PYTHON_PREFIX)/lib
NUMPY_INCLUDE ?= $(PYTHON_LIBDIR)/python$(PYTHON_VERSION)/site-packages/numpy/_core/include
MATPLOTLIBCPP_INCLUDE ?= $(PYTHON_INCLUDE)

PROJECT_CPPFLAGS = -Iinclude -isystem "$(PYTHON_INCLUDE)" -isystem "$(NUMPY_INCLUDE)" -isystem "$(MATPLOTLIBCPP_INCLUDE)"
PROJECT_CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -Wconversion
PROJECT_LDFLAGS = -L"$(PYTHON_LIBDIR)" -Wl,-rpath,"$(PYTHON_LIBDIR)"
PROJECT_LDLIBS = -lpython$(PYTHON_VERSION)
SANITIZERS = -fsanitize=address,undefined -fno-omit-frame-pointer

LIB_SOURCES = src/MertonModel.cpp src/Option.cpp src/MonteCarloPricer.cpp
APP_SOURCES = src/main.cpp $(LIB_SOURCES)
HEADERS = $(wildcard include/*.hpp)

.PHONY: all release debug run test clean

all: release

release: output_release

debug: output_debug

output_release: $(APP_SOURCES) $(HEADERS) Makefile
	$(CXX) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CXXFLAGS) $(PROJECT_CXXFLAGS) -O3 -DNDEBUG $(APP_SOURCES) $(LDFLAGS) $(PROJECT_LDFLAGS) $(LDLIBS) $(PROJECT_LDLIBS) -o $@

output_debug: $(APP_SOURCES) $(HEADERS) Makefile
	$(CXX) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CXXFLAGS) $(PROJECT_CXXFLAGS) -O0 -g $(APP_SOURCES) $(LDFLAGS) $(PROJECT_LDFLAGS) $(LDLIBS) $(PROJECT_LDLIBS) -o $@

run: release
	./output_release

build/regression: tests/regression.cpp $(LIB_SOURCES) $(HEADERS) Makefile
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CXXFLAGS) $(PROJECT_CXXFLAGS) -O1 -g $(SANITIZERS) tests/regression.cpp $(LIB_SOURCES) $(LDFLAGS) $(PROJECT_LDFLAGS) $(LDLIBS) $(PROJECT_LDLIBS) -o $@

test: build/regression
	ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ./build/regression

clean:
	$(RM) output_release output_debug build/regression
	$(RM) -r output_release.dSYM output_debug.dSYM build/regression.dSYM
