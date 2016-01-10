#ifndef COUNTER_H_
#define COUNTER_H_

#include <stdint.h>

struct counter_data {
	int range;

	uint64_t * stats;
	uint64_t count;
};

typedef struct counter_data COUNTER_DATA;

void counter_init_data(COUNTER_DATA * data, int range);
void counter_destroy_data(COUNTER_DATA * data);

int counter_filter(int number, void * data);

uint64_t counter_get(COUNTER_DATA * data, int number);

#endif /* COUNTER_H_ */
