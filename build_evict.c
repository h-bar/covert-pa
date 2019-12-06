#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sched.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <list.h>
#include <macros.h>
#include <page_set.h>

#include <clflush.h>
#include <tsx.h>

int cmp_size(const void *lhs, const void *rhs)
{
	return memcmp(lhs, rhs, sizeof(size_t));
}

int prime(struct list *working_set)
{
	struct list *node;
	unsigned ret;

	if ((ret = xbegin()) == XBEGIN_INIT) {
		list_foreach(working_set, node);
		xend();

		return 1;
	}

	return 0;
}

int prime_and_abort(struct list *working_set, void *line)
{
	struct list *node;
	size_t ncommits = 0, naborts = 0;
	unsigned ret;

	while (ncommits < 16 && naborts < 16) {
		if ((ret = xbegin()) == XBEGIN_INIT) {
			list_foreach(working_set, node);
			*(volatile char *)line;
			xend();

			++ncommits;
		} else if (ret & XABORT_CAPACITY) {
			// fprintf(stderr, "aboarted");
			++naborts;
		}
	}

	return ncommits == 0;
}

int main(int argc, char *argv[])
{
	struct list set;
	struct page_set lines, wset, eset;
	struct cache_line *line;
	char *page;
	char *target;
	size_t fast, slow;
	size_t count;
	size_t i;
	size_t npages = 512 * 2;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <count>\n",
			argv[0]);
		return -1;
	}

	count = (size_t)strtoull(argv[1], NULL, 10);

	if ((target = mmap(NULL, 4 * KIB, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
		fprintf(stderr, "error: unable to map target page\n");
		return -1;
	}

	memset(target, 0x5b, 4 * KIB);

	if ((page = mmap(NULL, npages * 4 * KIB, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
		fprintf(stderr, "error: unable to map pages for PRIME + PROBE\n");
		return -1;
	}

	memset(page, 0x5a, npages * 4 * KIB);
	page_set_init(&lines, npages);

	for (i = 0; i < npages; ++i) {
		page_set_push(&lines, page + i * 4 * KIB);
	}

	page_set_link(&lines, &set, (size_t)target & (4 * KIB - 1));
	if (!prime_and_abort(&set, target)) {
		fprintf(stderr, "error: mapped page does not map to target in cache\n");
		return 1;
	}

	page_set_init(&wset, 16);
	page_set_init(&eset, 1);
	page_set_clear(&eset);
			
	while (lines.len > 16) {
		line = page_set_remove(&lines, lines.len);
		page_set_link(&lines, &set, (size_t)target & (4 * KIB - 1));

		if (!prime_and_abort(&set, target)) {
			page_set_push(&lines, line);
		}
		printf("found eviction set of size: %zu\n", lines.len);
	}

	page_set_link(&eset, &set, (size_t)target & (4 * KIB - 1));
	if (prime_and_abort(&set, target)) {
		fprintf(stderr, "Aborted!\n");
		return 0;
	} else {
		fprintf(stderr, "error: Eviction set does not map to target in cache\n");
		return 1;
	}
 

	// 	for (i = wset.len; i > 0; --i) {
	// 		line = page_set_remove(&wset, i - 1);
	// 		page_set_link(&wset, &set, (size_t)target & (4 * KIB - 1));

	// 		if (prime_and_abort(&set, target)) {
	// 			page_set_push(&lines, line);
	// 		} else {
	// 			page_set_push(&wset, line);
	// 		}
	// 	}

	// 	printf("found working set of size: %zu\n", wset.len);
	// } while (wset.len > 17);

	// page_set_link(&wset, &set, (size_t)target & (4 * KIB - 1));

	// fast = 0;
	// slow = 0;

	// for (i = 0; i < count; ++i) {
	// 	prime(&set);
	// 	fast += prime(&set);
	// 	*(volatile char *)target;
	// 	slow += prime(&set);
	// }

	// printf("%zu %zu\n", slow, fast);

	return 0;
}
