#include <stdio.h>
#include <string.h>


#include <set>
#include <vector>
#include <algorithm>
#include <bitset>

#include "prime.cpp"
#include "eset.cpp"

using namespace std;


bitset<window_size> low_pattern(0);
vector<set<char *>> e_sets;

void sample(char ***e_addrs, bitset<window_size> *pattern_p) {
		*pattern_p <<= 1;
		if (prime_rate(e_addrs, _nways, sample_size) * 10 > h_threshold) {
			pattern_p->set(0, 1);
		} else {
			pattern_p->set(0, 0);
		}
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

int scan_cb(bitset<window_size> &pattern) {
	if (pattern == low_pattern) {
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

int server(int index) {
	e_sets = esets();
	int slice = scan(index);
	printf("Signal detected! Start listening on slices %d\n", slice);
	// monitor(e_sets[slice], index, 0);
}

int client(int index) {
	e_sets = esets();
	char **e_addrs = construct_addrs(e_sets[0], index);
	prime_on(&e_addrs, 0);
}

int help() {
	printf("usage: covert-pa [server|client] index\n");
	exit(1);
}

int main(int argc, char *argv[]) {
	int index = 1;
	if (argc < 2) help();
	else if (argc == 3) {
		index = atoi(argv[2]);
		if (index == 0) {
			printf("index 0 is reseverd for building confliction set, using default index: 3\n");
		}
	}

	if (strcmp(argv[1], "server") == 0) server(index); 
	else if (strcmp(argv[1], "client") == 0) client(index);
	else help();

	return 0;
}