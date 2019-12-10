#ifndef PRIME_CPP
#define PRIME_CPP


#include <immintrin.h>

#include "config.h"

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

float prime_rate(char ***addrs, int naddr, int ntries) {
	int nsuccess = 0;
	for (int i = 0; i < ntries; ++i) {
		if (pa_prime(*addrs, naddr)) {
			nsuccess ++;
		}
	}

	float rate = (float)nsuccess / (float)ntries;
	return rate;
}

int conflict_test(char ***addrs, int naddr, int ntries) {
	for (int i = 0; i < ntries; ++i) {
		if (pa_prime(*addrs, naddr)) {
			return 0;
		}
	}
	return 1;
}

char *prime_on(char *** e_addrs, int naddr, int nrepeats = 1) {
	// char **e_addrs = construct_addrs(e_set, index);
	char *pattern = (char *)malloc((nrepeats+1) * sizeof(char));
	int rate;
	int limit = nrepeats +1;
	int count = 0;
	while (count < nrepeats | nrepeats == 0) {
		rate = (int)(prime_rate(e_addrs, naddr, 1000) * 10);
		if (rate > 3) pattern[count] = '1';
		else 					pattern[count] = '0';
		count = (count + 1) % limit;
	}
	pattern[limit-1] = '\0';
	return pattern;
}

#endif