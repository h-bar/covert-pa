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
#define clock_cycle 5000000
#define buffer_size 1
#define sample_size 50000


#define nway_in			12
#define nway_out		12


#define scan_time   1000
#define l_thre      0.05

using namespace std;

bitset<buffer_size> l(string(buffer_size, '0')), h(string(buffer_size, '1'));
int clk, ss, mosi, miso;


 int sample(char ***e_addrs, int prev_v, int nsample) {
		return try_prime(*e_addrs, nway_in, nsample) > 0;
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

void init_wire_in(set<char *> e_set, int index, int nsample, int* value) {
	char **e_addrs = construct_addrs(e_set, index);
	while (1) {
		*value = sample(&e_addrs, *value, nsample);
	}
}

void init_wire_out(set<char *> eset, int index, int* value) {
	char **e_addrs = construct_addrs(eset, index);
	int reading;
	while(1) {
		if (*value == 1) {
			pa_prime(e_addrs, nway_out);
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

	printf("Master clock signal found on slice %d\n", slice);
	
	
	int mosi_value = 1;
	int ss_value = 1;
	int null_value = 1;
	thread mosi_wire (init_wire_in, e_sets[slice], mosi, sample_size, &mosi_value);
	thread null_wire (init_wire_in, e_sets[slice], clk+1, sample_size, &null_value);
	thread ss_wire(init_wire_out, e_sets[slice], ss, &ss_value);
	
	int count = 0;

	char **clk_addrs = construct_addrs(e_sets[slice], clk);
	
	bitset<buffer_size> clock_buffer(0);
	bitset<buffer_size> mosi_buffer(0);

	int clock_value = 0;
	int value_in = 0;
	while (1) {
		while (clock_value == 0) {
			clock_buffer <<= 1;
			clock_buffer[0] = sample(&clk_addrs, clock_buffer[1], sample_size);
			
			clock_value = clock_buffer.count() == 0;

			// mosi_buffer <<= 1;
			// mosi_buffer[0] = mosi_value;
			// value_in = mosi_buffer.count() == 0;
			// printf("0");
		}
		
		mosi_buffer <<= 1;
		mosi_buffer[0] = mosi_value;
		value_in = mosi_buffer.count() == 0;
		printf("%d", value_in);

		while (clock_value == 1) {
			clock_buffer <<= 1;
			clock_buffer[0] = sample(&clk_addrs, clock_buffer[1], sample_size);
			clock_value = clock_buffer.count() == 0;

			// printf("1");
		}
	}
}

int master() {
  set<char *> e_set = esets(0)[0];

	int mosi_value = 0;
	int ss_value = 1;
	thread mosi_wire(init_wire_out, e_set, mosi, &mosi_value);
	thread ss_wire(init_wire_in, e_set, ss, 5000, &ss_value);
	
	char **clk_addrs = construct_addrs(e_set, clk);
	char **null_addrs = construct_addrs(e_set, clk + 2);
	int count = 0;
	int timer = 0;
	int half_clock_cycle = clock_cycle / 2;
	int timeout = 4;
	while (1) {
		printf("Waiting for slave conntction\n");
		while(ss_value) pa_prime(clk_addrs, nway_out);

		printf("Slave connected, sending message: ");
		timer = 0;
		while(timer < timeout) {
			if (ss_value) timer++;
			else timer = 0;

			mosi_value = (rand() % 10 > 5);
			printf("%d", mosi_value);
			
			count = 0;
			while (count ++ < half_clock_cycle) {
				pa_prime(null_addrs, nway_out);
			}
			
			count = 0;
			while (count ++ < half_clock_cycle) {
				pa_prime(clk_addrs, nway_out);
			}
		}
		printf("\nSlave disconnected\n");
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