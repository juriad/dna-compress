#include <stdlib.h>
#include <math.h>

#include "common.h"
#include "arithmetic.h"

ARITHMETIC arithmetic_open(BINARIZER binarizer) {
	ARITHMETIC arithmetic = calloc(1, sizeof(*arithmetic));
	arithmetic->binarizer = binarizer;

	arithmetic->bits = sizeof(arithmetic->lower) * 8;

	uint32_t one = 1;
	arithmetic->b1 = one << (arithmetic->bits - 1);
	arithmetic->b2 = one << (arithmetic->bits - 2);

	arithmetic->lower = 0;
	arithmetic->range = arithmetic->b1;
	arithmetic->pending = 0;

	arithmetic->model.bit0 = 1;
	arithmetic->model.bit1 = 1;

	return arithmetic;
}

void arithmetic_close(ARITHMETIC arithmetic) {
	if (arithmetic->symbols > 0) {
		printf("Written %ld symbols\n", arithmetic->symbols);
		unsigned char space[8];
		convert_to_data(arithmetic->symbols, 8, space);
		fasta_write_space(arithmetic->binarizer->fasta, arithmetic->position, 8,
				space);
	}

	if (FASTA_IS_WRITING(arithmetic->binarizer->fasta)) {
		// output pending bits from lower
		//for (int i = 0; arithmetic->pending > 0; arithmetic->pending--, i++) {
		for (int i = 0; i < 64; i++) {
			int shift = arithmetic->bits - i - 1;
			int bit = (arithmetic->lower >> shift) & 1;
			binarizer_put_bit(arithmetic->binarizer, bit);
		}
	}

	free(arithmetic);
}

void output(ARITHMETIC arithmetic, int bit) {
	binarizer_put_bit(arithmetic->binarizer, bit);
	for (; arithmetic->pending > 0; arithmetic->pending--) {
		binarizer_put_bit(arithmetic->binarizer, 1 - bit);
	}
}

void arithmetic_encode_bit(ARITHMETIC arithmetic, int bit) {
	if (arithmetic->symbols == 0) {
		arithmetic->position = fasta_reserve_space(arithmetic->binarizer->fasta,
				8);
		printf("reserved position: %ld\n", arithmetic->position);
	}

	bit &= 1;
	double r = arithmetic->range
			/ (double) (arithmetic->model.bit0 + arithmetic->model.bit1);

	printf("encoding %d, lower is %016lX, range is %016lX, ratio is %f\n", bit,
			arithmetic->lower, arithmetic->range, r);

	if (bit) {
		arithmetic->lower += r * arithmetic->model.bit0;
		arithmetic->range -= r * arithmetic->model.bit0;
	} else {
		// lower stays the same
		arithmetic->range = r * arithmetic->model.bit0;
	}

	printf("            lower is %016lX, range is %016lX\n", arithmetic->lower,
			arithmetic->range);

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
		printf("         lower is %016lX, range is %016lX\n", arithmetic->lower,
				arithmetic->range);
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

		arithmetic->position = fasta_reserve_space(fasta, 8);
		unsigned char space[8];
		fasta_read_space(fasta, arithmetic->position, 8, space);
		arithmetic->symbols = convert_from_data(8, space);
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
			arithmetic->lower |= (bit & (uint64_t) 1) << shift;
		}
	}

	double r = arithmetic->range
			/ (double) (arithmetic->model.bit0 + arithmetic->model.bit1);

	printf("decoding,   lower is %016lX, range is %016lX, ratio is %f\n",
			arithmetic->lower, arithmetic->range, r);

	int decoded = 0;
	if (arithmetic->lower >= r * arithmetic->model.bit0) {
		decoded = 1;
		arithmetic->lower -= r * arithmetic->model.bit0;
		arithmetic->range -= r * arithmetic->model.bit0;
	} else {
		// lower stays the same
		arithmetic->range = r * arithmetic->model.bit0;
	}

	printf("%d+          lower is %016lX, range is %016lX\n", decoded,
			arithmetic->lower, arithmetic->range);

	while (arithmetic->range <= arithmetic->b2) {
		arithmetic->range <<= 1;
		arithmetic->lower <<= 1;
		// refill bits
		int bit = binarizer_get_bit(arithmetic->binarizer);
		if (bit >= 0) {
			printf("fill bit %d\n", bit & 1);
			arithmetic->lower |= ((uint64_t) bit & 1);
		} else {
			printf("out of bits\n");
		}
		printf("            lower is %016lX, range is %016lX\n",
				arithmetic->lower, arithmetic->range);
	}

	arithmetic->symbols--;
	return decoded;
}
