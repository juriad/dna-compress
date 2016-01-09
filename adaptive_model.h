#ifndef ADAPTIVE_MODEL_H_
#define ADAPTIVE_MODEL_H_

#include <stdint.h>

#include "arithmetic.h"

struct model_data {
	union {
		struct {
			double symbol0;
			double symbol1;
		};
		double symbols[2];
	};
	uint64_t cnt;
	uint64_t history;
	double degradation;
};

typedef struct model_data MODEL_DATA;

void adaptive_model_init(ARITHMETIC_MODEL * model, int history);
void adaptive_model_destroy(ARITHMETIC_MODEL * model);

arithmetic_type adaptive_model_rank(int symbol, arithmetic_type range, void * data);
int adaptive_model_select(arithmetic_type code, arithmetic_type range, void * data);
void adaptive_model_update(int symbol, void * data);

#endif /* ADAPTIVE_MODEL_H_ */
