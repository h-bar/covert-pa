#include <stdio.h>
#include <string.h>


#include <set>
#include <vector>
#include <algorithm>
#include <bitset>

#include "prime.cpp"
#include "eset.cpp"

using namespace std;
vector<set<char *>> e_sets;

int decode_rate(float rate, int prev_v) {
	return (prev_v == 1 && rate > l_thre) || (prev_v == 0 && rate > h_thre);
}

void sample(char ***e_addrs, bitset<window_size> *pattern_p) {
		*pattern_p <<= 1;
		(*pattern_p)[0] = decode_rate(prime_rate(e_addrs, _nways, sample_size), (*pattern_p)[1]);
}

int monitor(set<char *> e_set, int index, int time, int (*cb)(bitset<window_size> &pattern)) {
	int timer = 0;
	bitset<window_size> pattern(0);
	char **e_addrs = construct_addrs(e_set, index);

	while (timer < time || time == 0) {
		sample(&e_addrs, &pattern);
		if (!cb(pattern)) return 1;
		timer++;
	}

	return 0;
}

// int scan_cb(bitset<window_size> &pattern) {
// 	if (pattern == l) {
// 		return 0;
// 	}
// 	return 1;
// }

int scan(int index) {
	int slice = 0;
	while(!monitor(e_sets[slice], index, scan_time, &scan_cb)) {	
		printf("Scanned slice %d\n", slice);
		slice = (slice + 1) % e_sets.size();
	}

	return slice;
}

int server_cb(bitset<window_size> &pattern) {
	printf("%s\n", pattern.to_string().c_str());
	return 1;
}

int server(int index) {
	e_sets = esets();
	int slice = scan(index);
	printf("Signal detected! Start listening on slices %d\n", slice);
	monitor(e_sets[slice], index, 0, *server_cb);
}

int client(int index) {
	e_sets = esets();
	char **e_addrs = construct_addrs(e_sets[0], index);
	prime_on(&e_addrs, 0);
}

int help() {
	printf("usage: uart-pa clk-index gnd-index mosi-index miso-index\n");
	exit(1);
}

int main(int argc, char *argv[]) {
	if (argc < 6) help();
	
	int clk = atoi(argv[1]);
	int gnd = atoi(argv[2]);
	int mosi = atoi(argv[3]);
	int miso = atoi(argv[4]);
		
	if (clk == 0 || gnd == 0 || mosi == 0 || miso == 0) {
		printf("index 0 is reseverd for building confliction set\n");
	}

	// if (strcmp(argv[1], "server") == 0) server(index); 
	// else if (strcmp(argv[1], "client") == 0) client(index);
	// else help();

	return 0;
}