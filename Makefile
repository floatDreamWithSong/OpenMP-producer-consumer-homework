CXX = g++
CXXFLAGS = -fopenmp -std=c++11

all: main

main: main.c
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f main