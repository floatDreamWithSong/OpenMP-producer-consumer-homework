CXX = g++
CXXFLAGS = -fopenmp -std=c++11

all: producer_consumer

producer_consumer: main.cpp queue.h
	$(CXX) $(CXXFLAGS) main.cpp -o producer_consumer

clean:
	rm -f producer_consumer