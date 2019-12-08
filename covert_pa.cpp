#include <stdio.h>
#include <string.h>


#include <set>
#include <vector>
#include <algorithm>

#include "prime.cpp"
#include "eset.cpp"

using namespace std;

#define low_pattern "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"

int scan(vector<set<char *>>  e_sets, int index) {
	char *pattern;
	int slice;

	while(1) {	
		for (int slice = 0; slice < e_sets.size(); ++slice) {
			printf("Scanning on slice %d\t-> ", slice);
			char **e_addrs = construct_addrs(e_sets[slice], index);
			pattern = prime_on(&e_addrs, 100);
			printf("%s\n", pattern);
			if (strcmp(pattern, low_pattern) == 0) {
				return slice;
			}
		}
	}
}

void listen_to(set<char *> e_set, int index) {
	char *pattern;
	char result;
	
	char **e_addrs = construct_addrs(e_set, index);
	while(1) {	
		pattern = prime_on(&e_addrs, 100);
		if (strcmp(pattern, low_pattern) == 0)	result = '0';
		else 																		result = '1';
		printf("%c", result);
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

	if (strcmp(argv[1], "server") == 0) {
		int slice = scan(e_sets, index);
		printf("Signal recieved! Start listening on slices %d\n", slice);
		listen_to(e_sets[slice], index);
	} else if (strcmp(argv[1], "client") == 0) {
		char **e_addrs = construct_addrs(e_sets[0], index);
		prime_on(&e_addrs, 0);
	} else {
		printf("usage: %s [server|client]\n", argv[0]);
		return -1;
	}

	return 0;
}