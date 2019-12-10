#ifndef ESET_CPP
#define ESET_CPP

#include <string.h>
#include <sys/mman.h>

#include <set>
#include <vector>

#include "prime.cpp"
#include "config.h"

using namespace std;



char ** construct_addrs(set<char *> addr_set, int index = 0) {
	char **addrs = (char **)malloc(addr_set.size() * sizeof(char *));

	int i = 0;
	for (auto it = addr_set.begin(); it != addr_set.end(); ++it) {
		addrs[i] = *it + index * line_size;
		++i;
	}

	return addrs;
}

int init_page_array(char **page_array) {
	// initiaize a page array for 8 pages
	if ((*page_array = (char *)mmap(NULL, npages * page_size, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_HUGETLB | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
		printf("error: unable to map target page\n");
		return -1;
	}
	memset(*page_array, 0x5b, npages * page_size);
}

set<char *> creat_addr_set(char *&page_array) {
	// get number of address matching to same cache set index in all slices
	int naddrs = npages * page_size / way_size;
	set<char *> addr_set;
	// Build the address set with all the address pointing to same cache set
	for (int i = 0; i < naddrs; ++i) {
		addr_set.insert(page_array + i * way_size);
	}
	return addr_set;
}

int conflict_test(char ***addrs, int naddr, int ntries) {
	for (int i = 0; i < ntries; ++i) {
		if (pa_prime(*addrs, naddr)) {
			// if the current address is commit
			return 0;
		}
	}
	// otherwise aborted
	return 1;
}

vector<set<char *>> build_esets(set<char *> o_set) {
	set<char *> c_set;
	vector<set<char *>> e_sets = vector<set<char *>>();
	while (e_sets.size() < nslices) {
		// printf("Testing use working set of size %ld\n", c_set.size());
		char **w_addrs = construct_addrs(o_set);
		int naddr = o_set.size();
		// find conflict set in one slice
		for (int i = 0; i < naddr; ++i) {
			o_set.erase(w_addrs[i]);

			char **c_addrs = construct_addrs(c_set);
			if (!conflict_test(&c_addrs, c_set.size(), 1000)) {
				// if the erased address is committed
				// add it back to the conflict set
				c_set.insert(w_addrs[i]);
				o_set.insert(w_addrs[i]);
			}
			
		}
		// if none of the address is conflicted, 
		// which means we got all the conflict set, break.
		// if (c_set.size() == naddr) break;

		char **c_addrs = construct_addrs(c_set);
		if (!conflict_test(&c_addrs, c_set.size(), 10)) { continue; }
		
		e_sets.push_back(c_set);
		// remove the found address of one slice to find the rest slices
		for (auto o_it = o_set.begin(); o_it != o_set.end(); ++o_it) {
			for (auto w_it = c_set.begin(); w_it != c_set.end(); ++w_it) {
				if (*o_it == *w_it) o_set.erase(*w_it);
			}
		}
		//c_set = o_set;
	}
	printf("Found %ld eviction sets!\n", e_sets.size());
	return e_sets;
}

vector<set<char *>> esets() {
 	char *pages;	
	 // initiate a page array
	init_page_array(&pages);

	// Get a initial conflicted address set
	set<char *> o_set = creat_addr_set(pages);
	printf("Number of address: %ld\n", o_set.size());
	// Transform the set to array
	char **o_addrs = construct_addrs(o_set);

	// Check if the original set is a conflict set or not
	if (!conflict_test(&o_addrs, o_set.size(), 100000)) {
		printf("Original set is not a conflict set"); 
	}
	// Created the eviction sets
	vector<set<char *>> e_sets = vector<set<char *>>();
	while(e_sets.size() != nslices) {
		e_sets = build_esets(o_set);
	}
	// Eviction set size is nslices (16)
	return e_sets;
}

#endif