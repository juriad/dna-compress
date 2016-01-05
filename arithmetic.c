#include <stdlib.h>

#include "common.h"
#include "arithmetic.h"

#define RESERVED_SPACE 8

// TODO remove printf

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

	// TODO make model adaptive
	arithmetic->model.bit0 = 1;
	arithmetic->model.bit1 = 1;

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
		printf("Written %ld symbols\n", arithmetic->symbols);
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

	free(arithmetic);
}

void arithmetic_encode_bit(ARITHMETIC arithmetic, int bit) {
	if (arithmetic->symbols == 0) {
		arithmetic->position = fasta_reserve_space(arithmetic->binarizer->fasta,
		RESERVED_SPACE);
		printf("reserved position: %ld\n", arithmetic->position);
	}

	bit &= 1;
	arithmetic_type r = arithmetic->range
			/ (double) (arithmetic->model.bit0 + arithmetic->model.bit1);

	printf(
			"encoding %d, lower is " ARITHMETIC_TYPE_FORMAT ", range is " ARITHMETIC_TYPE_FORMAT
			", ratio is " ARITHMETIC_TYPE_FORMAT ", part is " ARITHMETIC_TYPE_FORMAT "\n",
			bit, arithmetic->lower, arithmetic->range, r,
			r * arithmetic->model.bit0);

	if (bit) {
		arithmetic->lower += r * arithmetic->model.bit0;
		arithmetic->range -= r * arithmetic->model.bit0;
	} else {
		// lower stays the same
		arithmetic->range = r * arithmetic->model.bit0;
	}

	printf(
			"            lower is " ARITHMETIC_TYPE_FORMAT ", range is " ARITHMETIC_TYPE_FORMAT "\n",
			arithmetic->lower, arithmetic->range);

	while (arithmetic->range <= arithmetic->b2) {
		// tests which deals with integer overflow
		if (arithmetic->lower + arithmetic->range <= arithmetic->b1
				&& arithmetic->lower < arithmetic->lower + arithmetic->range) { // 0
			printf("0+ ");
			output(arithmetic, 0);
		} else if (arithmetic->b1 <= arithmetic->lower) { // 1
			printf("1+ ");
			output(arithmetic, 1);
			arithmetic->lower -= arithmetic->b1;
		} else {
			printf("p+ ");
			arithmetic->pending++;
			arithmetic->lower -= arithmetic->b2;
		}
		arithmetic->lower <<= 1;
		arithmetic->range <<= 1;
		printf(
				"         lower is " ARITHMETIC_TYPE_FORMAT ", range is " ARITHMETIC_TYPE_FORMAT "\n",
				arithmetic->lower, arithmetic->range);
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
		printf("read about %ld symbols on position %ld\n", arithmetic->symbols,
				arithmetic->position);
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
			printf("init bit %d\n", bit & 1);
			int shift = arithmetic->bits - i - 1;
			arithmetic->lower |= (bit & (arithmetic_type) 1) << shift;
		}
	}

	arithmetic_type r = arithmetic->range
			/ (double) (arithmetic->model.bit0 + arithmetic->model.bit1);

	printf(
			"decoding,   lower is " ARITHMETIC_TYPE_FORMAT ", range is " ARITHMETIC_TYPE_FORMAT
			", ratio is " ARITHMETIC_TYPE_FORMAT ", part is " ARITHMETIC_TYPE_FORMAT "\n",
			arithmetic->lower, arithmetic->range, r,
			r * arithmetic->model.bit0);

	int decoded = 0;
	if (arithmetic->lower >= r * arithmetic->model.bit0) {
		decoded = 1;
		arithmetic->lower -= r * arithmetic->model.bit0;
		arithmetic->range -= r * arithmetic->model.bit0;
	} else {
		// lower stays the same
		arithmetic->range = r * arithmetic->model.bit0;
	}

	printf(
			"%d+          lower is " ARITHMETIC_TYPE_FORMAT ", range is " ARITHMETIC_TYPE_FORMAT
			"\n", decoded, arithmetic->lower, arithmetic->range);

	while (arithmetic->range <= arithmetic->b2) {
		arithmetic->range <<= 1;
		arithmetic->lower <<= 1;
		// refill bits
		int bit = binarizer_get_bit(arithmetic->binarizer);
		if (bit >= 0) {
			arithmetic->lower |= ((arithmetic_type) bit & 1);
		} else {
		}
		printf(
				"            lower is " ARITHMETIC_TYPE_FORMAT ", range is " ARITHMETIC_TYPE_FORMAT
				", filled %d\n", arithmetic->lower, arithmetic->range,
				bit >= 0 ? bit & 1 : bit);
	}

	arithmetic->symbols--;
	return decoded;
}
