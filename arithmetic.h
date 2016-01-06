#ifndef ARITHMETIC_H_
#define ARITHMETIC_H_

#include <stdint.h>

#include "binarizer.h"

typedef uint64_t arithmetic_type;
#define ARITHMETIC_TYPE_FORMAT "%d"//"%02hhX"

struct arithmetic {
	BINARIZER binarizer;
	off_t position;
	uint64_t symbols;

	struct {
		union {
			struct {
				double bit0;
				double bit1;
			};
			double bits[2];
		};
		uint64_t cnt;
		uint64_t history;
		double degradation;
	} model;

	arithmetic_type lower;
	arithmetic_type range;
	arithmetic_type pending;
	arithmetic_type zeros;

	char bits;
	arithmetic_type b1;
	arithmetic_type b2;
};

typedef struct arithmetic * ARITHMETIC;

ARITHMETIC arithmetic_open(BINARIZER binarizer);
void arithmetic_close(ARITHMETIC arithmetic);

void arithmetic_encode_bit(ARITHMETIC arithmetic, int bit);
int arithmetic_decode_bit(ARITHMETIC arithmetic);

#endif /* ARITHMETIC_H_ */
