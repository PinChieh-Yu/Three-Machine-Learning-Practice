all:
	g++ -std=c++11 -O3 -g -Wall -fmessage-length=0 -o Three Three.cpp
clean:
	rm Three