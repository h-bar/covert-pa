#ifndef CONFIG_H
#define CONFIG_H

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
#define npages      8

#define window_size 1000
#define sample_size 100
#define h_threshold 1
#define scan_time   1000

#endif