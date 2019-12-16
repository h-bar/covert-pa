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

int try_prime(char **addrs, int size, int ntries)
{
	unsigned ret;
	int nsuccess = 0;
	for (int i = 0; i < ntries; ++i) {
		if ((ret = _xbegin()) == _XBEGIN_STARTED) {
			for (int i = 0; i < size; ++i) {
				*(volatile char *)addrs[i];
			}
			_xend();

			nsuccess ++;
		}
	}

	return nsuccess;
}

int conflict_test(char ***addrs, int naddr, int ntries) {
	for (int i = 0; i < ntries; ++i) {
		if (pa_prime(*addrs, naddr)) {
			return 0;
		}
	}
	return 1;
}

#endif