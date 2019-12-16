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
#define wire_threshold 0
#define clock_threshold 0

#define nway_in			12
#define nway_out		12


#define scan_time   1000
#define scan_sample_size 	100
#define scan_threshold		0

using namespace std;

bitset<buffer_size> l(string(buffer_size, '0')), h(string(buffer_size, '1'));
int clk, ss, mosi, miso;


 int sample(char ***e_addrs, int nsample, int threshold) {
		return try_prime(*e_addrs, nway_in, nsample) > threshold;
}

int scan(vector<set<char *>> e_sets, int index) {
	int slice = 0, timer = 0;
	char **e_addrs;
	while(1) {	
		printf("Scanning slice %d\n", slice);
		e_addrs = construct_addrs(e_sets[slice], index);
		while (timer < scan_time) {
			if (sample(&e_addrs, scan_sample_size, scan_threshold) == 0) return slice;
			timer++;
		}
		timer = 0;
		slice = (slice + 1) % e_sets.size();
	}
}

void init_wire_in(set<char *> e_set, int index, int nsample, int* value) {
	char **e_addrs = construct_addrs(e_set, index);
	while (1) {
		*value = sample(&e_addrs, nsample, wire_threshold);
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
			clock_buffer[0] = sample(&clk_addrs, sample_size, clock_threshold);
			
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
			clock_buffer[0] = sample(&clk_addrs, sample_size, clock_threshold);
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