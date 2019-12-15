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
#define clock_cycle 1500000000	
#define _nways			12


#define buffer_size 1
#define sample_size 10000
#define scan_time   1000
#define l_thre      0.05

using namespace std;

bitset<buffer_size> l(string(buffer_size, '0')), h(string(buffer_size, '1'));
int clk, ss, mosi, miso;

void pull_down(char ***e_addrs) { pa_prime(*e_addrs, _nways); }
void pull_up() { for(int i = 0; i < 37; i++) {} }

int sample(char ***e_addrs, int prev_v, int nsample) {
		float rate = prime_rate(e_addrs, _nways, nsample);
		int value = rate >= l_thre;
		return value;
}

int monitor(set<char *> e_set, int index, int time) {
	int timer = 0;
	bitset<buffer_size> pattern(0);
	char **e_addrs = construct_addrs(e_set, index);

	while (timer < time || time == 0) {
		pattern <<= 1;
		pattern[0] = sample(&e_addrs, pattern[1], 100);

		if (pattern == l) return 1;
		timer++;
	}

	return 0;
}

int scan(vector<set<char *>> e_sets, int index) {
	int slice = 0;
	while(!monitor(e_sets[slice], index, scan_time)) {	
		printf("Scanned slice %d\n", slice);
		slice = (slice + 1) % e_sets.size();
	}

	return slice;
}

void init_wire_in(set<char *> e_set, int index, int* value) {
	char **e_addrs = construct_addrs(e_set, index);
	while (1) {
		*value = sample(&e_addrs, *value, sample_size);
	}
}

void init_wire_out(set<char *> eset, int index, int* value) {
	char **e_addrs = construct_addrs(eset, index);
	int reading;
	while(1) {
		if (*value == 1) {
			reading = sample(&e_addrs, reading, sample_size);
		} else {
			pull_up();
		}
	}
}

int help() {
	printf("usage: spi-master clk-index ss-index mosi-index miso-index\n");
	exit(1);
}

int slave() {
	vector<set<char *>> e_sets = esets(0);
	int slice = scan(e_sets, clk);

	printf("Clock signal found on slice %d\n", slice);
	
	
	int clock_value = 1;
	int mosi_value = 1;
	thread clock_wire (init_wire_in, e_sets[slice], clk, &clock_value);
	thread mosi_wire (init_wire_in, e_sets[slice], mosi, &mosi_value);
	int count = 0;
	while (1) {
		bitset<buffer_size> clock_buffer(0);
		bitset<buffer_size> mosi_buffer(0);

		int prev_clk = 0;
		int current_clk = 0;
		int value_in = 0;
		while (1) {
			clock_buffer <<= 1;
			clock_buffer[0] = clock_value;

			mosi_buffer <<= 1;
			mosi_buffer[0] = mosi_value;
			// printf("%s\n", buffer.to_string().c_str());
			if (clock_buffer == l) {current_clk = 1;}
			else if (clock_buffer == h) {current_clk = 0;}


			if (mosi_buffer == l) {value_in = 1;}
			else if (mosi_buffer == h) {value_in = 0;}
			// float rate = (float)(buffer.count()) / buffer_size;
			// value = rate > 0.2;
			
			if ((prev_clk - current_clk) == -1) {
				printf("%d", value_in);
			}

			prev_clk = current_clk;
		}
	}
}

int master() {
  set<char *> e_set = esets(0)[0];

	int clock_value = 1;
	int mosi_value = 1;
	int ss_value = 0;
	thread clock_wire(init_wire_out, e_set, clk, &clock_value);
	thread mosi_wire(init_wire_out, e_set, mosi, &mosi_value);
	thread ss_wire(init_wire_out, e_set, ss, &ss_value);
	int count = 0;
	while (1) {
		int half_clock_cycle = clock_cycle / 2;
		printf("Clock started on index %d\n", clk);

		while(1) {
			mosi_value = (rand() % 10 > 5);
			printf("%d", mosi_value);
			while (count++ < half_clock_cycle) {}
			clock_value = 1;
			while (count-- >= 0) {}
			clock_value = 0;
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc < 5) help();
	
	setbuf(stdout, NULL);
	clk = atoi(argv[2]);
	ss = atoi(argv[3]);
	mosi = atoi(argv[4]);
		
	if (clk == 0 || ss == 0 || mosi == 0) {
		printf("index 0 is reseverd for building confliction set\n");
	}

	if (argv[1][0] == 'm') master();
	else if (argv[1][0] == 's') slave();
	return 0;
}