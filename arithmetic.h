#ifndef ARITHMETIC_H_
#define ARITHMETIC_H_

#include <stdint.h>

#include "binarizer.h"

struct arithmetic {
	BINARIZER binarizer;
	size_t position;
	uint64_t symbols;

	struct {
		uint32_t bit0;
		uint32_t bit1;
		uint32_t cnt;
	} model;

	char bits;
	uint32_t lower;
	uint32_t range;
	uint32_t pending;

	uint32_t b1;
	uint32_t b2;
};

typedef struct arithmetic * ARITHMETIC;

ARITHMETIC arithmetic_open(BINARIZER binarizer);

void arithmetic_close(ARITHMETIC arithmetic);

void arithmetic_encode_bit(ARITHMETIC arithmetic, int bit);

int arithmetic_decode_bit(ARITHMETIC arithmetic);

#endif /* ARITHMETIC_H_ */
