#ifndef ARITHMETIC_H_
#define ARITHMETIC_H_

#include <stdint.h>

#include "binarizer.h"

struct arithmetic {
	BINARIZER binarizer;
	size_t position;
	uint64_t symbols;

	struct {
		uint64_t bit0;
		uint64_t bit1;
		uint64_t cnt;
	} model;

	uint64_t lower;
	uint64_t range;
	uint64_t pending;

	uint64_t b1;
	uint64_t b2;
};

typedef struct arithmetic * ARITHMETIC;

ARITHMETIC arithmetic_open(BINARIZER binarizer);

void arithmetic_close(ARITHMETIC arithmetic);

void arithmetic_encode_bit(ARITHMETIC arithmetic, int bit);

int arithmetic_decode_bit(ARITHMETIC arithmetic);

#endif /* ARITHMETIC_H_ */
