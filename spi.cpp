#include <stdio.h>
#include <string.h>


#include <set>
#include <vector>
#include <algorithm>
#include <bitset>
#include <atomic> 
#include <thread>

#include "prime.cpp"
#include "eset.cpp"

#define pull_down 	pa_prime(*e_addrs, _nways)
#define pull_up			for(int i = 0; i < 26; i++) {}

using namespace std;

atomic_flag ticking = ATOMIC_FLAG_INIT;

bitset<window_size> l(string(window_size, '0')), h(string(window_size, '1'));
vector<set<char *>> e_sets;
	
int clk, gnd, mosi, miso;
char **clk_addrs, **gnd_addrs, **mosi_addrs, **miso_addrs;

int decode_rate(float rate, int prev_v) {
	return (prev_v == 1 && rate > l_thre) || (prev_v == 0 && rate > h_thre);
}

void sample(char ***e_addrs, bitset<window_size> *pattern_p) {
		*pattern_p <<= 1;
		(*pattern_p)[0] = decode_rate(prime_rate(e_addrs, _nways, sample_size), (*pattern_p)[1]);
}


int monitor(set<char *> e_set, int index, int time, int (*cb)(bitset<window_size> &pattern, int timer)) {
	int timer = 0;
	bitset<window_size> pattern(0);
	char **e_addrs = construct_addrs(e_set, index);

	while (timer < time || time == 0) {
		sample(&e_addrs, &pattern);
		if (!cb(pattern, timer)) return 1;
		timer++;
	}

	return 0;
}


// int scan(int index) {
// 	printf("Scanning all slices for low signals on index %d\n", index);
// 	thread scan_threads[e_sets.size()];
// 	bitset<window_size> steams[e_sets.size()];
// 	int stop = 0;
// 	for (int i = 0; i < e_sets.size(); i++) {
// 		char **e_addrs = construct_addrs(e_sets[i], index);
// 		steams[i] = bitset<window_size>(0);
// 		scan_threads[i] = thread(monitor, &e_addrs, clk, &(steams[i]), &stop);
// 	}

// 	int slice;
// 	while (1) {
// 		for (slice = 0; slice < e_sets.size(); slice++) {
// 			if (steams[slice] == l) {
// 				// stop = 1;
// 				for (int i = 0; i < e_sets.size(); i++) {
// 					scan_threads[i].join();
// 				}
// 				return slice;
// 			}
// 		}
// 	}

// }


int scan_cb(bitset<window_size> &pattern, int timer) {
	if (pattern == l) {
		return 0;
	}
	return 1;
}

int scan(int index) {
	int slice = 0;
	while(!monitor(e_sets[slice], index, scan_time, &scan_cb)) {	
		printf("Scanned slice %d\n", slice);
		slice = (slice + 1) % e_sets.size();
	}

	return slice;
}

int print_cb(bitset<window_size> &pattern,  int timer) {
	if (timer % 2000 == 0) {
		printf("%s\n", pattern.to_string().c_str());
	}
	return 1;
}


int sample(char ***e_addrs, int prev_v) {
		return decode_rate(prime_rate(e_addrs, _nways, sample_size), prev_v);
}


void monitor_asyc(char ***e_addrs, int *stop, bitset<window_size> *bitstream) {
	while(*stop == 0) {
		(*bitstream) <<= 1;
		(*bitstream)[0] = sample(e_addrs, (*bitstream)[1]);
	}
}

void clocking(char ***e_addrs) {
	int count = 0;
	int half_clock_cycle = clock_cycle / 2;
	printf("Clock started on index %d\n", clk);

	while(1) {
		while (count < half_clock_cycle) {
			pull_down;
			count++;
		}
		while (count >= 0) {
			pull_up;
			count--;
		}
		ticking.clear();
	}
}

void gound(char ***e_addrs) {
	printf("GND signal started on index %d\n", gnd);
	while (1) {
		pull_down;
	}
}

int master() {
	e_sets = esets();

	clk_addrs = construct_addrs(e_sets[0], clk);
	gnd_addrs = construct_addrs(e_sets[0], gnd);
	mosi_addrs = construct_addrs(e_sets[0], mosi);
	miso_addrs = construct_addrs(e_sets[0], miso);

	
	thread gnd_generator (gound, &gnd_addrs);
	thread clock_generator (clocking, &clk_addrs);
	int count = 0;
	while (1) {
		while(ticking.test_and_set()) {
			
		}
		count++;
		printf("A clock tick \n");
	}
}

int slave() {
	e_sets = esets();
	int slice = scan(clk);

	printf("Clock signal found on slice %d\n", slice);
	
	clk_addrs = construct_addrs(e_sets[slice], clk);
	gnd_addrs = construct_addrs(e_sets[slice], gnd);
	mosi_addrs = construct_addrs(e_sets[slice], mosi);
	miso_addrs = construct_addrs(e_sets[slice], miso);
	
	bitset<window_size> clock_stream(0);
	int stop;
	thread clock_thread (monitor_asyc, &clk_addrs, &stop, &clock_stream);
	clock_thread.join();
	// monitor(e_sets[slice], clk, 0, print_cb);
}

int help() {
	printf("usage: uart-pa [master|slave] clk-index gnd-index mosi-index miso-index\n");
	exit(1);
}

int main(int argc, char *argv[]) {
	// if (argc < 6) help();
	
	clk = atoi(argv[2]);
	gnd = atoi(argv[3]);
	// mosi = atoi(argv[4]);
	// miso = atoi(argv[5]);
		
	if (clk == 0 || gnd == 0 || mosi == 0 || miso == 0) {
		printf("index 0 is reseverd for building confliction set\n");
	}

	if (strcmp(argv[1], "master") == 0) master(); 
	else if (strcmp(argv[1], "slave") == 0) slave();
	else help();

	return 0;
}