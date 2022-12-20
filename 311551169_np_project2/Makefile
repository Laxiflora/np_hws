all: np_simple.cpp np_single_proc.cpp
	g++ np_simple.cpp -O0 -g -o np_simple
	g++ np_single_proc.cpp -O0 -g -o np_single_proc
	g++ np_multi_proc.cpp -O0 -g -lrt -pthread -o np_multi_proc

clean:
	rm np_simple