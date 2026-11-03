# ЛР4: C++17 + OpenMP. macOS: Apple clang + libomp з Homebrew (brew install libomp).
# Linux / MinGW: замініть CXX на g++, а OMPFLAGS на -fopenmp.
CXX      = clang++
LIBOMP   = $(shell brew --prefix libomp 2>/dev/null)
OMPFLAGS = -Xpreprocessor -fopenmp -I$(LIBOMP)/include -L$(LIBOMP)/lib -lomp
CXXFLAGS = -O2 -std=c++17 -Wall

PROGRAMS = hello histogram

all: $(addprefix bin/,$(PROGRAMS))

bin/%: src/%.cpp src/common.hpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@ $(OMPFLAGS)

clean:
	rm -rf bin

.PHONY: all clean
