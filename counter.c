#include <stdlib.h>

#include "counter.h"

void counter_init_data(COUNTER_DATA * data, int range) {
	data->range = range;
	data->stats = calloc(range, sizeof(*data->stats));
	data->count = 0;
}

void counter_destroy_data(COUNTER_DATA * data) {
	free(data->stats);
}

int counter_filter(int number, void * data_) {
	COUNTER_DATA * data = data_;

	data->stats[number % data->range]++;
	data->count++;

	return number;
}

uint64_t counter_get(COUNTER_DATA * data, int number) {
	return data->stats[number % data->range];
}
