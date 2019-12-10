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

#define clock_cycle 150	
#define _nways			12

using namespace std;

atomic_flag ticking = ATOMIC_FLAG_INIT;

int clk, gnd, mosi, miso;
char **clk_addrs, **gnd_addrs, **mosi_addrs, **miso_addrs;

void pull_down(char ***e_addrs) { pa_prime(*e_addrs, _nways); }
void pull_up() { for(int i = 0; i < 37; i++) {} }

void clocking(char ***e_addrs) {
	int count = 0;
	int half_clock_cycle = clock_cycle / 2;
	printf("Clock started on index %d\n", clk);

	while(1) {
		while (count < half_clock_cycle) {
			pull_down(e_addrs);
			count++;
		}
		while (count >= 0) {
			pull_up();
			count--;
		}
		ticking.clear();
	}
}

void gound(char ***e_addrs) {
	printf("GND signal started on index %d\n", gnd);
	while (1) {
		pull_down(e_addrs);
	}
}

int help() {
	printf("usage: spi-master clk-index gnd-index mosi-index miso-index\n");
	exit(1);
}

int master() {
  vector<set<char *>> e_sets = esets(0);

	clk_addrs = construct_addrs(e_sets[0], clk);
	gnd_addrs = construct_addrs(e_sets[0], gnd);
	mosi_addrs = construct_addrs(e_sets[0], mosi);
	miso_addrs = construct_addrs(e_sets[0], miso);

	
	// thread gnd_generator (gound, &gnd_addrs);
	thread clock_generator (clocking, &clk_addrs);
	int count = 0;

	string sequence = "sfdsfsddsfsadsfsafsafd";
	int nbits = sequence.length() * sizeof(char);
	int pos = 0;
	int value = 0;
	char **e_addrs = mosi_addrs;
	while (1) {
		// value = (sequence[pos / sizeof(char)] & (1 << pos % sizeof(char)) == 0);
		while(ticking.test_and_set()) {
		// 	if (value) {
		// 		pull_down(&mosi_addrs);
		// 	} else {
		// 		pull_up();
		// 	}
		}
		// pos = (pos + 1) / nbits;
		count++;
		printf("A clock tick at time %d\n", count);
	}
}

int main(int argc, char *argv[]) {
	if (argc < 5) help();
	
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