#include <stdlib.h>
#include <math.h>

#include "common.h"

#include "adaptive_model.h"

void adaptive_model_init(ARITHMETIC_MODEL * model, int history) {
	// TODO generalize for more symbols
	model->model_rank = adaptive_model_rank;
	model->model_select = adaptive_model_select;
	model->model_update = adaptive_model_update;

	MODEL_DATA * data = model->model_data = calloc(1, sizeof(*data));

	data->symbol0 = 1;
	data->symbol1 = 1;

	data->history = history;
	data->degradation = pow(0.5, 1.0 / data->history);
}

void adaptive_model_destroy(ARITHMETIC_MODEL * model) {
	free(model->model_data);
}

arithmetic_type adaptive_model_rank(int symbol, arithmetic_type range,
		void * data_) {
	if (symbol == 0) {
		return 0;
	} else if (symbol == 2) {
		return range;
	}

	MODEL_DATA * data = data_;

	double sum = data->symbol0 + data->symbol1;
	double nom = range * data->symbol0;
	arithmetic_type r0 = nom / sum;
	r0 = RANGE(r0, 1, range - 1);
	return r0;
}

int adaptive_model_select(arithmetic_type code, arithmetic_type range,
		void * data_) {
	arithmetic_type rank1 = adaptive_model_rank(1, range, data_);
	if (code < rank1) {
		return 0;
	} else {
		return 1;
	}
}

void adaptive_model_update(int symbol, void * data_) {
	MODEL_DATA * data = data_;

	data->cnt++;

	for (int i = 0; i < 2; i++) {
		data->symbols[i] *= data->degradation;
	}

	if (data->cnt % 10000 == 0) {
		printf("symbol1: %f, symbol0: %f\n", data->symbol1, data->symbol0);
	}

	data->symbols[symbol]++;
}

