#include <stdio.h>
#include <string.h>
#include <time.h>

#include <set>
#include <vector>
#include <algorithm>
#include <bitset>
#include <atomic> 
#include <thread>
#include <random>


#include "prime.cpp"
#include "eset.cpp"

// #define clock_cycle 15000
// #define sample_size_clk 500
// #define sample_threshold_clk 50
// #define buffer_size 10


#define clock_cycle 100
#define sample_size_clk 10
#define sample_threshold_clk 1
#define buffer_size 10




#define sample_size_mosi 10
#define sample_threshold_mosi 1

#define settle_time 350000
#define sample_size_ss 10
#define sample_threshold_ss 1


// #define buffer_size 100
#define buffer_threshold 20


#define nway_in			8
#define nway_out		12


#define scan_time   1000
#define scan_sample_size 	100
#define scan_threshold		5

#define nbytes_to_send 738
using namespace std;

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

void init_wire_in(set<char *> e_set, int index, int nsample, int threshold, bitset<buffer_size> *buffer) {
	char **e_addrs = construct_addrs(e_set, index);
	while (1) {
		(*buffer) <<= 1;
		(*buffer)[0] = sample(&e_addrs, nsample, threshold);
	}
}

void init_wire_out(set<char *> eset, int index, int* value) {
	char **e_addrs = construct_addrs(eset, index);
	while(1) {
		if (*value == 1) {
			pa_prime(e_addrs, nway_out);
		}
	}
}

int help() {
	printf("usage: spi [m|s] clk-index ss-index mosi-index\n");
	exit(1);
}

int slave() {
	vector<set<char *>> e_sets = esets(0);
	int slice = scan(e_sets, ss);

	printf("Master signal found on slice %d\n", slice);
	FILE *fp = fopen("../test/out.txt", "w");
	if(fp == NULL) {
		perror("Error in opening file");
		return(-1);
	} 

	bitset<buffer_size> mosi_buffer(0);
	bitset<buffer_size> ss_buffer(0);
	bitset<buffer_size> clk_buffer(0);

	thread mosi_wire (init_wire_in, e_sets[slice], mosi, sample_size_mosi, sample_threshold_mosi, &mosi_buffer);
	thread ss_wire(init_wire_in, e_sets[slice], ss, sample_size_ss, sample_threshold_ss, &ss_buffer);
	
	int nbytes = 0;
	int counter = 0;

	char **clk_addrs = construct_addrs(e_sets[slice], clk);
	
	bitset<1000> data_buffer(0);


	int _count = 0;
	while (1) {

		do {
			clk_buffer <<= 1;
			clk_buffer[0] = sample(&clk_addrs, sample_size_clk, sample_threshold_clk);
		} while (clk_buffer.count() == buffer_size);
		printf("%d\n", ++_count);
		// printf("%d", sample(&clk_addrs, sample_size_clk, sample_threshold_clk));
		do {
				clk_buffer <<= 1;
				clk_buffer[0] = sample(&clk_addrs, sample_size_clk, sample_threshold_clk);
			} while (clk_buffer.count() < buffer_size);
	}









	char data_recieved[nbytes_to_send];
	while (nbytes < nbytes_to_send) {
		while (ss_buffer.count() < buffer_size - buffer_threshold) {}
		while (ss_buffer.count() > buffer_threshold) {
			do {
				clk_buffer <<= 1;
				clk_buffer[0] = sample(&clk_addrs, sample_size_clk, sample_threshold_clk);
			} while (clk_buffer.count() > buffer_threshold && ss_buffer.count() > buffer_threshold);

			data_buffer[counter] = (mosi_buffer.count() < buffer_size - buffer_threshold);
			counter ++;
			do {
				clk_buffer <<= 1;
				clk_buffer[0] = sample(&clk_addrs, sample_size_clk, sample_threshold_clk);
			} while (clk_buffer.count() < buffer_size - buffer_threshold && ss_buffer.count() > buffer_threshold);
		}

		data_recieved[nbytes] = (char)data_buffer.to_ulong();
		printf("%c", data_recieved[nbytes]);
		data_buffer.reset();
		counter = 0;
		nbytes++;
	}
	// printf("All data recieved\n");
	// fprintf(fp, "%s", data_recieved);
	// fflush(fp);
	// fclose(fp);
	// printf("%s\n", data_recieved);
	while(1);
}

int master() {
  set<char *> e_set = esets(0)[0];
	
	FILE *fp = fopen("../test/file.txt", "r");
	if(fp == NULL) {
		perror("Error in opening file");
		return(-1);
	}

	int mosi_value = 0;
	int ss_value = 1;
	thread mosi_wire(init_wire_out, e_set, mosi, &mosi_value);
	thread ss_wire(init_wire_out, e_set, ss, &ss_value);
	
	char **clk_addrs = construct_addrs(e_set, clk);
	char **null_addrs = construct_addrs(e_set, clk + 2);
	int count = 0;
	int nbytes = 0;
	char data = 0;
	char data_sent[nbytes_to_send];
	int half_clock_cycle = clock_cycle / 2;
	int i, j;




	

	while(1) {
		printf("Press enter to start transmission");
		getchar();
		

		unsigned long start_t_ = (unsigned long)time(NULL);
		int _counter = 0;
		while (_counter < 500000) {
			count = 0;
			while (count ++ < half_clock_cycle) {
				pa_prime(null_addrs, nway_out);
			}
			_counter ++;
			count = 0;
			while (count ++ < half_clock_cycle) {
				pa_prime(clk_addrs, nway_out);
			}
		}
		unsigned long end_t_ = (unsigned long)time(NULL);
		printf("50000 cycles sent in %lu seconds -> %f sec\n", (end_t_ - start_t_), (float)(end_t_ - start_t_) / 50000);
	}

	while (1) {
		pa_prime(clk_addrs, nway_out);
	}

	unsigned long start_t = (unsigned long)time(NULL);
	while (!feof(fp)) {
		ss_value = 0;
		for (i = 0; i < 1; i++) {
			data = getc(fp);
			for (j = 0; j < 8; j++) {
				mosi_value = data & 1;
				count = 0;
				while (count ++ < half_clock_cycle) {
					pa_prime(null_addrs, nway_out);
				}
				
				count = 0;
				while (count ++ < half_clock_cycle) {
					pa_prime(clk_addrs, nway_out);
				}
				data >>= 1;
			}
			nbytes++;
		}
		ss_value = 1;

		count = 0;
		while (count ++ < settle_time) {}
	}
	unsigned long end_t = (unsigned long)time(NULL);
	printf("%d bytes sent in %lu seconds -> %f bytes/sec\n", nbytes, (end_t - start_t), (float)nbytes / (end_t - start_t));
	while(1);

}

int main(int argc, char *argv[]) {
	if (argc != 5) help();
	
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