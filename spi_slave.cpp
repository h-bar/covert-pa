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


using namespace std;


#define buffer_size 200
#define sample_size 10
#define window_size 2
#define scan_time   1000
#define l_thre      0.05
#define h_thre      0.4

#define _nways			12

#define pull_down 	pa_prime(*e_addrs, _nways)
#define pull_up			for(int i = 0; i < 26; i++) {}



int window_0 = 0;
int window_1 = 2^window_size - 1;
bitset<buffer_size> l(string(buffer_size, '0')), h(string(buffer_size, '1'));
vector<set<char *>> e_sets;
	
int clk, gnd, mosi, miso;
char **clk_addrs, **gnd_addrs, **mosi_addrs, **miso_addrs;



int sample(char ***e_addrs, int prev_v) {
		float rate = prime_rate(e_addrs, _nways, sample_size);
		int value = rate >= l_thre;
		// printf("%f -> %d: %s\n", rate, value, string((int)(rate*10), '=').c_str());
		return value;
}



int monitor(set<char *> e_set, int index, int time) {
	int timer = 0;
	bitset<buffer_size> pattern(0);
	char **e_addrs = construct_addrs(e_set, index);

	while (timer < time || time == 0) {
		pattern <<= 1;
		pattern[0] = sample(&e_addrs, pattern[1]);

		if (pattern == l) return 1;
		timer++;
	}

	return 0;
}

int monitor2(set<char *> e_set, int index, int time) {
	int timer = 0;
	bitset<buffer_size> buffer(0);

	char **e_addrs = construct_addrs(e_set, index);

	int prev_v = 0;
	int value = 0;
	int count = 0;
	while (timer < time || time == 0) {
		buffer <<= 1;
		buffer[0] = sample(&e_addrs, buffer[1]);

		float rate = (float)((buffer >> (buffer_size - window_size)).count()) / window_size;
		// float rate = (float)((buffer).count()) / buffer_size;
		value = rate > 0.2;
		// printf("%f -> %d: %s\n", rate, value, string((int)(rate*10), '=').c_str());
		
		if ((prev_v - value) == 1) {
			printf("A tick at time %d!\n", count);
			count++;
		}

		


		prev_v = value;
		timer++;
	}

	return 0;
}

int scan(int index) {
	int slice = 0;
	while(!monitor(e_sets[slice], index, scan_time)) {	
		printf("Scanned slice %d\n", slice);
		slice = (slice + 1) % e_sets.size();
	}

	return slice;
}


void monitor_asyc(char ***e_addrs, int *stop, bitset<buffer_size> *bitstream) {
	while(*stop == 0) {
		(*bitstream) <<= 1;
		(*bitstream)[0] = sample(e_addrs, (*bitstream)[1]);
	}
}

int slave() {
	e_sets = esets(0);
	int slice = scan(clk);

	printf("Clock signal found on slice %d\n", slice);
	
	clk_addrs = construct_addrs(e_sets[slice], clk);
	gnd_addrs = construct_addrs(e_sets[slice], gnd);
	mosi_addrs = construct_addrs(e_sets[slice], mosi);
	miso_addrs = construct_addrs(e_sets[slice], miso);
	
	monitor2(e_sets[slice], clk, 0);
}


int help() {
	printf("usage: spi-slave clk-index gnd-index mosi-index miso-index\n");
	exit(1);
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

	slave();
	return 0;
}