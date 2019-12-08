// #include <ctype.h>
// #include <inttypes.h>
// #include <stdint.h>
// #include <stdlib.h>


// #include <fcntl.h>
// #include <sched.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/stat.h>

#include <stdio.h>
#include <string.h>

#include <sys/mman.h>
#include <immintrin.h>

#include <set>
#include <vector>
#include <algorithm>

using namespace std;

#define _XBEGIN_STARTED (~0u) 
#define _XABORT_EXPLICIT (1 << 0) 
#define _XABORT_RETRY (1 << 1) 
#define _XABORT_CONFLICT (1 << 2) 
#define _XABORT_CAPACITY (1 << 3) 
#define _XABORT_DEBUG (1 << 4) 
#define _XABORT_NESTED (1 << 5)

int pa_prime(char **addrs, int size)
{
	unsigned ret;

	if ((ret = _xbegin()) == _XBEGIN_STARTED) {
		for (int i = 0; i < size; ++i) {
			*(volatile char *)addrs[i];
		}
		_xend();

		return 1;
	}

	return 0;
}

int prime_and_abort(char **eviction, int size, char *target)
{
	size_t ncommits = 0, naborts = 0;
	unsigned ret;

	while (ncommits < 16 && naborts < 16) {
		if ((ret = _xbegin()) == _XBEGIN_STARTED) {
			*(volatile char *)target;
			for (int i = 0; i < size; ++i) {
				*(volatile char *)eviction[i];
			}
			_xend();

			return 0;
		} else if (ret & _XABORT_CAPACITY) {
			++naborts;
		}
	}

	return 1;
}

float prime_test(char ***addrs, int naddr, int ntries) {
	int nsuccess = 0;
	for (int i = 0; i < ntries; ++i) {
		if (pa_prime(*addrs, naddr)) {
			nsuccess ++;
		}
	}

	float rate = (float)nsuccess / (float)ntries;
	// printf("Prime success rate = %f\n", rate);
	return rate;
}

int conflict_test(char ***addrs, int naddr, int ntries) {
	for (int i = 0; i < ntries; ++i) {
		if (pa_prime(*addrs, naddr)) {
			// printf("This is not a conflict set\n");
			return 0;
		}
	}
	return 1;
}

// int pa_test(set<char *> addr_set, char *&target, int ntries) {
// 	char **search_addrs = construct_addrs(addr_set);
// 	for (int i = 0; i < ntries; ++i) {
// 		if (!prime_and_abort(search_addrs, addr_set.size(), target)) {
// 			printf("Prime + Abort failed\n");
// 			return 1;
// 		}
// 	}
// 	printf("Prime + Abort succeed\n");
// 	return 0;
// }


#define BIT(n) (1 << (n))

#define B		((size_t)1)
#define KIB (1024 * B)
#define MIB (1024 * KIB)
#define GIB (1024 * MIB)

#define cache_size 	(16 * MIB)
#define line_size		64 * B
#define nways 			16
#define _nways			12
#define nslices 		16
#define way_size 		(cache_size / nslices / nways)

#define page_size 	(2 * MIB)

char ** construct_addrs(set<char *> addr_set, int index = 0) {
	char **addrs = (char **)malloc(addr_set.size() * sizeof(char *));

	int i = 0;
	for (auto it = addr_set.begin(); it != addr_set.end(); ++it) {
		addrs[i] = *it + index * line_size;
		++i;
	}

	return addrs;
}

int init_page_array(char **page_array, int npages) {
	if ((*page_array = (char *)mmap(NULL, npages * page_size, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_HUGETLB | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
		printf("error: unable to map target page\n");
		return -1;
	}
	memset(*page_array, 0x5b, npages * page_size);
}

set<char *> creat_addr_set(char *&page_array, int npages) {
	int naddrs = npages * page_size / way_size;
	set<char *> addr_set;
	for (int i = 0; i < naddrs; ++i) {
		addr_set.insert(page_array + i * way_size);
	}
	printf("Number of address: %ld\n", addr_set.size());
	return addr_set;
}



vector<set<char *>> build_esets(set<char *> o_set) {
	set<char *> c_set = o_set;
	vector<set<char *>> e_sets = vector<set<char *>>();
	while (1) {
		// printf("Testing use working set of size %ld\n", c_set.size());
		char **w_addrs = construct_addrs(c_set);
		int naddr = c_set.size();

		for (int i = 0; i < naddr; ++i) {
			c_set.erase(w_addrs[i]);

			char **c_addrs = construct_addrs(c_set);
			if (!conflict_test(&c_addrs, c_set.size(), 1000)) {
				c_set.insert(w_addrs[i]);
			}
			
		}

		if (c_set.size() == naddr) break;

		char **c_addrs = construct_addrs(c_set);
		if (!conflict_test(&c_addrs, c_set.size(), 100000)) { continue; }
		// printf("Found conflict set of size %ld\n", c_set.size());
		e_sets.push_back(c_set);
		
		for (auto o_it = o_set.begin(); o_it != o_set.end(); ++o_it) {
			for (auto w_it = c_set.begin(); w_it != c_set.end(); ++w_it) {
				if (*o_it == *w_it) o_set.erase(*w_it);
			}
		}
		c_set = o_set;
	}
	printf("Found %ld conflict sets!\n", e_sets.size());
	return e_sets;
}

void listen_on(char ***e_addrs) {
	while(1) {
		int rate = (int)(prime_test(e_addrs, _nways, 1000) * 10);
		printf("%0*d\n", rate, 0);
	}
}

void send_to(char ***e_addrs_0, char ***e_addrs_1) {	
	int count = 0;
	int timer = 1;
	while(timer != 50) {
		while (count < 1000) {
			prime_test(e_addrs_0, _nways, 1000);
			count ++;
		}
		while (count >= 0) {
			prime_test(e_addrs_1, _nways, 1000);
			count --;
		}
		timer ++;
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

  char *pages;	
	init_page_array(&pages, npages);
	set<char *> o_set = creat_addr_set(pages, npages);
	
	char **o_addrs = construct_addrs(o_set);
	if (!conflict_test(&o_addrs, o_set.size(), 100000)) {
		printf("Original set is not a conflict set"); 
	}

	vector<set<char *>> e_sets = vector<set<char *>>();
	while(e_sets.size() != nslices) {
		e_sets = build_esets(o_set);
	}
	
	char **e_addr_0 = construct_addrs(e_sets[0], index);
	char **e_addr_1 = construct_addrs(e_sets[1], index);

	if (strcmp(argv[1], "server") == 0) {
		printf("Start listening on cache index %d", index);
		listen_on(&e_addr_0);	
	} else if (strcmp(argv[1], "client") == 0) {
		send_to(&e_addr_0, &e_addr_1);
	} else if (strcmp(argv[1], "slice_broadcast") == 0) {
		for (auto it = e_sets.begin(); it != e_sets.end(); ++it) {
			e_addr_0 = construct_addrs(*it, index);
			e_addr_1 = construct_addrs(*(++it), index);
			send_to(&e_addr_0, &e_addr_1);
		}
	} else {
		printf("usage: %s [server|client]\n", argv[0]);
		return -1;
	}

	return 0;
}