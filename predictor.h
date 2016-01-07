#ifndef PREDICTOR_H_
#define PREDICTOR_H_

#include <stdint.h>

struct predictor_data {
	char decoding;
	char length;
	uint64_t history;
	uint64_t ** frequencies;
};

typedef struct predictor_data PREDICTOR_DATA;

void predictor_init_data(PREDICTOR_DATA * data, char length, char decoding);
void predictor_destroy_data(PREDICTOR_DATA * data);

int predictor_filter(int bit, void * data);

#endif /* PREDICTOR_H_ */
