#include <stdlib.h>

#include "predictor.h"

void predictor_init_data(PREDICTOR_DATA * data, char length, char decoding) {
	data->length = length;
	data->decoding = decoding;
	data->history = 0;

	data->frequencies = calloc(length + 1, sizeof(*data->frequencies));
	for (int i = 0; i <= data->length; i++) {
		data->frequencies[i] = calloc(1 << (i + 1),
				sizeof(*data->frequencies[i]));
	}
}

void predictor_destroy_data(PREDICTOR_DATA * data) {
	for (int i = 0; i <= data->length; i++) {
		free(data->frequencies[i]);
	}

	free(data->frequencies);
	data->frequencies = NULL;
}

int predict1(PREDICTOR_DATA * data, int length) {
	uint64_t mask = (1 << length) - 1;
	uint64_t val = (data->history & mask) << 1;

	uint64_t bit0 = data->frequencies[length][val];
	uint64_t bit1 = data->frequencies[length][val | 1];

	if (bit0 == 0 && bit1 == 0) {
		return -1;
	}

	int predicted = bit0 >= bit1 ? 0 : 1;
	return predicted;
}

int predict(PREDICTOR_DATA * data) {
	int predicted = 0;
	for (int i = data->length; i >= 0; i--) {
		int bit = predict1(data, i);
		if (bit >= 0) {
			predicted = bit;
			break;
		}
	}
	return predicted;
}

void update1(PREDICTOR_DATA * data, int length, int actual) {
	uint64_t mask = (1 << length) - 1;
	uint64_t val = ((data->history & mask) << 1) | actual;
	data->frequencies[length][val]++;
}

void update(PREDICTOR_DATA * data, int actual) {
	for (int i = data->length; i >= 0; i--) {
		update1(data, i, actual);
	}
	data->history = (data->history << 1) | actual;
}

int predictor_filter(int bit, void * data_) {
	PREDICTOR_DATA * data = data_;

	int predicted = predict(data);

	bit &= 1;

	int actual;
	int ret;
	if (data->decoding) {
		actual = predicted ^ bit;
		ret = actual;
	} else {
		actual = bit;
		ret = predicted ^ actual;
	}

	update(data, actual);

	return ret;
}
