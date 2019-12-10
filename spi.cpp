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

int master() {
	e_sets = esets();

	clk_addrs = construct_addrs(e_sets[0], clk);
	gnd_addrs = construct_addrs(e_sets[0], gnd);
	mosi_addrs = construct_addrs(e_sets[0], mosi);
	miso_addrs = construct_addrs(e_sets[0], miso);

	std::thread clock_thread (clocking, &clk_addrs);
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
	printf("Clock signal found on\n");
	monitor(e_sets[slice], clk, 0, print_cb);
}

int help() {
	printf("usage: uart-pa [master|slave] clk-index gnd-index mosi-index miso-index\n");
	exit(1);
}

int main(int argc, char *argv[]) {
	// if (argc < 6) help();
	
	clk = atoi(argv[2]);
	// gnd = atoi(argv[3]);
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