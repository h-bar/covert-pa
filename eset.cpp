#ifndef ESET_CPP
#define ESET_CPP

#include <string.h>
#include <sys/mman.h>

#include <set>
#include <vector>

#include "prime.cpp"
#include "config.h"

using namespace std;



char ** construct_addrs(set<char *> addr_set, int index) {
	char **addrs = (char **)malloc(addr_set.size() * sizeof(char *));

	int i = 0;
	for (auto it = addr_set.begin(); it != addr_set.end(); ++it) {
		addrs[i] = *it + index * line_size;
		++i;
	}

	return addrs;
}

int init_page_array(char **page_array) {
	if ((*page_array = (char *)mmap(NULL, npages * page_size, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_HUGETLB | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
		printf("error: unable to map target page\n");
		return -1;
	}
	memset(*page_array, 0x5b, npages * page_size);
}

set<char *> creat_addr_set(char *&page_array) {
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
		char **w_addrs = construct_addrs(c_set, 0);
		int naddr = c_set.size();

		for (int i = 0; i < naddr; ++i) {
			c_set.erase(w_addrs[i]);

			char **c_addrs = construct_addrs(c_set, 0);
			if (!conflict_test(&c_addrs, c_set.size(), 1000)) {
				c_set.insert(w_addrs[i]);
			}
			
		}

		if (c_set.size() == naddr) break;

		char **c_addrs = construct_addrs(c_set, 0);
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

vector<set<char *>> esets() {
 char *pages;	
	init_page_array(&pages);
	set<char *> o_set = creat_addr_set(pages);
	
	char **o_addrs = construct_addrs(o_set, 0);
	if (!conflict_test(&o_addrs, o_set.size(), 100000)) {
		printf("Original set is not a conflict set"); 
	}

	vector<set<char *>> e_sets = vector<set<char *>>();
	while(e_sets.size() != nslices) {
		e_sets = build_esets(o_set);
	}

	return e_sets;
}

#endif