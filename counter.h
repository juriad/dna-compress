#ifndef COUNTER_H_
#define COUNTER_H_

#include <stdint.h>

struct counter_data {
	int min;
	int max;

	uint64_t * stats;
	uint64_t count;
};

typedef struct counter_data COUNTER_DATA;

void predictor_init_data(COUNTER_DATA * data, int min, int max);
void predictor_destroy_data(COUNTER_DATA * data);

int counter_filter(int number, void * data);

#endif /* COUNTER_H_ */
