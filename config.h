#ifndef CONFIG_H
#define CONFIG_H

#define BIT(n) (1 << (n))

#define B		((size_t)1)
#define KIB (1024 * B)
#define MIB (1024 * KIB)
#define GIB (1024 * MIB)

#define cache_size 	(12 * MIB)
#define line_size		64 * B
#define nways 			12
#define nslices 		16
#define way_size 		(cache_size / nslices / nways)

#define page_size 	(2 * MIB)
#define npages      8



#endif