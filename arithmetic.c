#include <stdlib.h>
#include <math.h>

#include "common.h"

#include "arithmetic.h"

#define RESERVED_SPACE 8

ARITHMETIC arithmetic_open(BINARIZER binarizer) {
	ARITHMETIC arithmetic = calloc(1, sizeof(*arithmetic));
	arithmetic->binarizer = binarizer;

	arithmetic->bits = sizeof(arithmetic_type) * 8;

	arithmetic_type one = 1;
	arithmetic->b1 = one << (arithmetic->bits - 1);
	arithmetic->b2 = one << (arithmetic->bits - 2);

	arithmetic->lower = 0;
	arithmetic->range = arithmetic->b1;
	arithmetic->pending = 0;
	arithmetic->zeros = 0;

	arithmetic->model.bit0 = 1;
	arithmetic->model.bit1 = 1;

	arithmetic->model.history = 100;
	arithmetic->model.degradation = pow(0.5, 1.0 / arithmetic->model.history);

	return arithmetic;
}

void write(ARITHMETIC arithmetic, int bit) {
	// prevent writing of trailing zeros
	if (bit == 0) {
		arithmetic->zeros++;
		return;
	}

	for (; arithmetic->zeros > 0; arithmetic->zeros--) {
		binarizer_put_bit(arithmetic->binarizer, 0);
	}
	binarizer_put_bit(arithmetic->binarizer, 1);
}

void output(ARITHMETIC arithmetic, int bit) {
	write(arithmetic, bit);
	for (; arithmetic->pending > 0; arithmetic->pending--) {
		write(arithmetic, 1 - bit);
	}
}

void arithmetic_close(ARITHMETIC arithmetic) {
	if (arithmetic->symbols > 0) {
		// output pending
		unsigned char space[RESERVED_SPACE];
		convert_to_data(arithmetic->symbols, RESERVED_SPACE, space);
		fasta_write_space(arithmetic->binarizer->fasta, arithmetic->position,
		RESERVED_SPACE, space);
	}

	if (FASTA_IS_WRITING(arithmetic->binarizer->fasta)) {
		// output bits from lower
		for (int i = 0; i < arithmetic->bits; i++) {
			int shift = arithmetic->bits - i - 1;
			int bit = (arithmetic->lower >> shift) & 1;
			output(arithmetic, bit);
		}
	}

	printf("bit1: %f, bit0: %f\n", arithmetic->model.bit1,
			arithmetic->model.bit0);

	free(arithmetic);
}

void update_model(ARITHMETIC arithmetic, int bit) {
	// TODO make better adaptive model
	arithmetic->model.cnt++;

	for (int i = 0; i < 2; i++) {
		arithmetic->model.bits[i] *= arithmetic->model.degradation;
	}

	arithmetic->model.bits[bit]++;
}

arithmetic_type range0(ARITHMETIC arithmetic) {
	double sum = arithmetic->model.bit0 + arithmetic->model.bit1;
	double nom = arithmetic->range * arithmetic->model.bit0;
	arithmetic_type r0 = nom / sum;
	r0 = RANGE(r0, 1, arithmetic->range - 1);
	return r0;
}

void arithmetic_encode_bit(ARITHMETIC arithmetic, int bit) {
	if (arithmetic->symbols == 0) {
		arithmetic->position = fasta_reserve_space(arithmetic->binarizer->fasta,
		RESERVED_SPACE);
	}

	bit &= 1;
	arithmetic_type r0 = range0(arithmetic);

	if (bit) {
		arithmetic->lower += r0;
		arithmetic->range -= r0;
	} else {
		// lower stays the same
		arithmetic->range = r0;
	}

	update_model(arithmetic, bit);

	while (arithmetic->range <= arithmetic->b2) {
		// deals with integer overflow
		if (arithmetic->lower + arithmetic->range <= arithmetic->b1
				&& arithmetic->lower < arithmetic->lower + arithmetic->range) { // 0
			output(arithmetic, 0);
		} else if (arithmetic->b1 <= arithmetic->lower) { // 1
			output(arithmetic, 1);
			arithmetic->lower -= arithmetic->b1;
		} else { // p
			arithmetic->pending++;
			arithmetic->lower -= arithmetic->b2;
		}
		arithmetic->lower <<= 1;
		arithmetic->range <<= 1;
	}
	arithmetic->symbols++;
}

int arithmetic_decode_bit(ARITHMETIC arithmetic) {
	if (arithmetic->symbols == 0) {
		if (arithmetic->position > 0) {
			// position can never be zero because of how fasta is organized
			// this happens when we are past the end
			return -1;
		}

		FASTA fasta = arithmetic->binarizer->fasta;
		if (!fasta_has_sequence(fasta)) {
			return -1;
		}

		arithmetic->position = fasta_reserve_space(fasta, RESERVED_SPACE);
		unsigned char space[RESERVED_SPACE];
		fasta_read_space(fasta, arithmetic->position, RESERVED_SPACE, space);
		arithmetic->symbols = convert_from_data(RESERVED_SPACE, space);
		if (arithmetic->symbols == 0) {
			// weird, but may happen
			return -1;
		}

		// read bits into lower
		for (int i = 0; i < arithmetic->bits; i++) {
			int bit = binarizer_get_bit(arithmetic->binarizer);
			if (bit < 0) {
				break;
			}
			int shift = arithmetic->bits - i - 1;
			arithmetic_type b = bit & 1;
			arithmetic->lower |= b << shift;
		}
	}

	arithmetic_type r0 = range0(arithmetic);

	int decoded = 0;
	if (arithmetic->lower >= r0) {
		decoded = 1;
		arithmetic->lower -= r0;
		arithmetic->range -= r0;
	} else {
		// lower stays the same
		arithmetic->range = r0;
	}

	update_model(arithmetic, decoded);

	while (arithmetic->range <= arithmetic->b2) {
		arithmetic->range <<= 1;
		arithmetic->lower <<= 1;
		// refill bits
		int bit = binarizer_get_bit(arithmetic->binarizer);
		if (bit >= 0) {
			arithmetic->lower |= bit & 1;
		}
	}

	arithmetic->symbols--;
	return decoded;
}
