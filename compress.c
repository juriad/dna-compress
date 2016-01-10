#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "fasta.h"
#include "binarizer.h"
#include "arithmetic.h"
#include "predictor.h"
#include "adaptive_model.h"
#include "counter.h"

#define PREDICTOR_LENGTH 22

void toggle(char * in, char * out) {
	FASTA_PTR f1 = fasta_open(in, FASTA_READING);
	FASTA_PTR f2 = fasta_open(out, FASTA_WRITING);

	BINARIZER_ALPHABET ident, acgt, a0123;
	binarizer_alphabet_identity(&ident);
	binarizer_alphabet_acgt(&acgt);
	binarizer_alphabet_0123(&a0123);

	while (1) {
		char * name = fasta_get_name(f1);
		if (name == NULL) {
			break;
		}

		name[0] = name[0] == '<' ? '>' : '<';
		fasta_put_name(f2, name);
		printf("%s\n", name);

		if (fasta_has_sequence(f1)) {
			BINARIZER_PTR b1, b2;

			ARITHMETIC_PTR a;
			ARITHMETIC_MODEL model;
			adaptive_model_init(&model, 30);
			PREDICTOR_DATA data;
			COUNTER_DATA before, after;
			counter_init_data(&before, 2);
			counter_init_data(&after, 2);

			if (name[0] == '<') {
				// encoding

				b1 = binarizer_open(f1, acgt);
				binarizer_add_filter(b1, counter_filter, &before);
				predictor_init_data(&data, PREDICTOR_LENGTH, 0);
				binarizer_add_filter(b1, predictor_filter, &data);
				binarizer_add_filter(b1, counter_filter, &after);
				b2 = binarizer_open(f2, ident);

				a = arithmetic_open(b2, model);

				int bit;
				while (1) {
					bit = binarizer_get_bit(b1);
					if (bit == -1) {
						break;
					}

					arithmetic_encode_symbol(a, bit);
				}
			} else {
				// decoding

				b1 = binarizer_open(f1, ident);
				b2 = binarizer_open(f2, a0123);
				binarizer_add_filter(b2, counter_filter, &before);
				predictor_init_data(&data, PREDICTOR_LENGTH, 1);
				binarizer_add_filter(b2, predictor_filter, &data);
				binarizer_add_filter(b2, counter_filter, &after);

				a = arithmetic_open(b1, model);

				int bit;
				while (1) {
					bit = arithmetic_decode_symbol(a);
					if (bit == -1) {
						break;
					}

					binarizer_put_bit(b2, bit);
				}
			}

			printf("0: %d, 1: %d -> 0: %d, 1: %d\n", counter_get(&before, 0),
					counter_get(&before, 1), counter_get(&after, 0),
					counter_get(&after, 1));

			arithmetic_close(a);
			adaptive_model_destroy(&model);
			binarizer_close(b1);
			binarizer_close(b2);
			predictor_destroy_data(&data);
			counter_destroy_data(&before);
			counter_destroy_data(&after);
		}

		free(name);

		int prevs = f1->curSeq;
		int s = fasta_seek_name(f1, 1);
		if (prevs == s) {
			break;
		}
	}

	fasta_close(f1);
	fasta_close(f2);
}

int main(int argc, char ** argv) {
	toggle(argv[1], argv[2]);
	exit(0);
}
