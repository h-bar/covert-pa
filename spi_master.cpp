#include <stdio.h>
#include <string.h>


#include <set>
#include <vector>
#include <algorithm>
#include <bitset>
#include <atomic> 
#include <thread>
#include <random>

#include "prime.cpp"
#include "eset.cpp"

// #define clock_cycle 150	
#define clock_cycle 150000000	
#define _nways			12

using namespace std;

int clk, gnd, mosi, miso;

void pull_down(char ***e_addrs) { pa_prime(*e_addrs, _nways); }
void pull_up() { for(int i = 0; i < 37; i++) {} }

void init_wire_out(set<char *> eset, int index, int* value) {
	char **e_addrs = construct_addrs(eset, index);
	while(1) {
		if (*value == 1) {
			pull_down(&e_addrs);
		} else {
			pull_up();
		}
	}
}

int help() {
	printf("usage: spi-master clk-index gnd-index mosi-index miso-index\n");
	exit(1);
}

int master() {
  vector<set<char *>> e_sets = esets(0);

	int clock_value = 1;
	int mosi_value = 1;
	thread clock_wire(init_wire_out, e_sets[0], clk, &clock_value);
	thread mosi_wire(init_wire_out, e_sets[0], mosi, &mosi_value);

	int count = 0;
	while (1) {
		int half_clock_cycle = clock_cycle / 2;
		printf("Clock started on index %d\n", clk);

		while(1) {
			clock_value = 1;
			while (count++ < half_clock_cycle) {}
			clock_value = 0;
			while (count-- >= 0) {}
			printf("t");
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc < 5) help();
	
	setbuf(stdout, NULL);
	clk = atoi(argv[1]);
	gnd = atoi(argv[2]);
	mosi = atoi(argv[3]);
	miso = atoi(argv[4]);
		
	if (clk == 0 || gnd == 0 || mosi == 0 || miso == 0) {
		printf("index 0 is reseverd for building confliction set\n");
	}

	master();
	return 0;
}