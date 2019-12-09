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

void sample(char ***e_addrs, bitset<window_size> *pattern_p) {
		*pattern_p <<= 1;
		if ((int)(prime_rate(e_addrs, _nways, sample_size) * 10) > h_threshold) {
			pattern_p->set(0, 1);
		} else {
			pattern_p->set(0, 0);
		}
}

void monitor(set<char *> e_set, int index, int time, bitset<window_size> *pattern_p) {
	int timer = 0;
	char **e_addrs = construct_addrs(e_set, index);

	while (timer < time || time == 0) {
		sample(&e_addrs, pattern_p);
		timer++;
	}
}

int scan(vector<set<char *>>  e_sets, int index, bitset<window_size> *pattern_p) {
	while(1) {	
		for (int slice = 0; slice < e_sets.size(); ++slice) {
			printf("Scanning on slice %d\n", slice);
			monitor(e_sets[slice], index, scan_time, pattern_p);
			if ((*pattern_p) == low_pattern) {
				return slice;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	int npages = 8;

	int index = 1;
	if (argc < 2) {
		printf("usage: %s [server|client] index\n", argv[0]);
		return -1;
	} else if (argc == 3) {
		index = atoi(argv[2]);
		if (index == 0) {
			printf("index 0 is reseverd for building confliction set");
		}
	}

  vector<set<char *>> e_sets = esets(npages);
	bitset<window_size> pattern(0);

	if (strcmp(argv[1], "server") == 0) {
		int slice = scan(e_sets, index, &pattern);
		printf("Signal recieved! Start listening on slices %d\n", slice);
		monitor(e_sets[slice], index, 0, &pattern);
	} else if (strcmp(argv[1], "client") == 0) {
		char **e_addrs = construct_addrs(e_sets[0], index);
		prime_on(&e_addrs, 0);
	} else {
		printf("usage: %s [server|client]\n", argv[0]);
		return -1;
	}

	return 0;
}