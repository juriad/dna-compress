#include <stdlib.h>
#include <math.h>

#include "common.h"

#include "arithmetic.h"

#define RESERVED_SPACE 8

ARITHMETIC arithmetic_open(BINARIZER binarizer, ARITHMETIC_MODEL model) {
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

	arithmetic->model = model;

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

	free(arithmetic);
}

void arithmetic_encode_symbol(ARITHMETIC arithmetic, int symbol) {
	if (arithmetic->symbols == 0) {
		arithmetic->position = fasta_reserve_space(arithmetic->binarizer->fasta,
		RESERVED_SPACE);
	}

	arithmetic_type rmz1 = arithmetic->model.model_rank(symbol,
			arithmetic->range, arithmetic->model.model_data);
	arithmetic->lower += rmz1;
	arithmetic_type rmz = arithmetic->model.model_rank(symbol + 1,
			arithmetic->range, arithmetic->model.model_data);
	arithmetic->range = rmz - rmz1;

	arithmetic->model.model_update(symbol, arithmetic->model.model_data);

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

int arithmetic_decode_symbol(ARITHMETIC arithmetic) {
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

	int symbol = arithmetic->model.model_select(arithmetic->lower,
			arithmetic->range, arithmetic->model.model_data);

	arithmetic_type rmz1 = arithmetic->model.model_rank(symbol,
			arithmetic->range, arithmetic->model.model_data);
	arithmetic->lower -= rmz1;
	arithmetic_type rmz = arithmetic->model.model_rank(symbol + 1,
			arithmetic->range, arithmetic->model.model_data);
	arithmetic->range = rmz - rmz1;

	arithmetic->model.model_update(symbol, arithmetic->model.model_data);

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
	return symbol;
}
