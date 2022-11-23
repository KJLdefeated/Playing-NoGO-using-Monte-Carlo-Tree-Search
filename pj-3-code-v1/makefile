all:
	g++ -std=c++11 -O3 -g -Wall -fmessage-length=0 -o nogo nogo.cpp
multithreads:
	g++ -std=c++11 -O3 -g -Wall -fmessage-length=0 -o nogo nogo.cpp -lpthread
run_stats:
	./nogo --save=stats.txt
clean:
	rm nogo