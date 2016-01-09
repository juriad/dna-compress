#include <stdlib.h>

#include "counter.h"

void counter_init_data(COUNTER_DATA * data, int min, int max) {
	data->min = min;
	data->max = max;

	int len = max - min + 1;
	data->stats = calloc(len, sizeof(*data->stats));
	data->count = 0;
}

void counter_destroy_data(COUNTER_DATA * data) {
	free(data->stats);
}

int counter_filter(int number, void * data_) {
	COUNTER_DATA * data = data_;

	data->stats[number - data->min]++;
	data->count++;

	return number;
}
