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


#define low_pattern "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"

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

char *prime_on(set<char *> e_set, int index, int nrepeats = 1) {
	char **e_addrs = construct_addrs(e_set, index);
	char *pattern = (char *)malloc((nrepeats+1) * sizeof(char));
	int rate;
	int limit = nrepeats +1;
	int count = 0;
	while (count < nrepeats | nrepeats == 0) {
		rate = (int)(prime_test(&e_addrs, _nways, 1000) * 10);
		if (rate > 3) pattern[count] = '1';
		else 					pattern[count] = '0';
		count = (count + 1) % limit;
	}
	pattern[limit-1] = '\0';
	return pattern;
}

int scan(vector<set<char *>>  e_sets, int index) {
	char *pattern;
	int slice;

	while(1) {	
		for (int slice = 0; slice < e_sets.size(); ++slice) {
			printf("Scanning on slice %d\t-> ", slice);
			pattern = prime_on(e_sets[slice], index, 100);
			printf("%s\n", pattern);
			if (strcmp(pattern, low_pattern) == 0) {
				return slice;
			}
		}
	}
}

void listen_to(set<char *> e_set, int index) {
	char *pattern;

	while(1) {	
		pattern = prime_on(e_set, index, 100);
		printf("%s\n", pattern);
		if (strcmp(pattern, low_pattern) == 0) {
			// return slice;
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
	

	if (strcmp(argv[1], "server") == 0) {
		int slice = scan(e_sets, index);
		printf("Signal recieved! Start listening on slices %d\n", slice);
		listen_to(e_sets[slice], index);
	} else if (strcmp(argv[1], "client") == 0) {
		prime_on(e_sets[0], index, 0);
	} else {
		printf("usage: %s [server|client]\n", argv[0]);
		return -1;
	}

	return 0;
}