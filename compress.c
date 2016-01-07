#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "fasta.h"
#include "binarizer.h"
#include "arithmetic.h"
#include "predictor.h"

#define PREDICTOR_LENGTH 22

void encode(char * in, char * out) {
	FASTA f1 = fasta_open(in, FASTA_READING);
	FASTA f2 = fasta_open(out, FASTA_WRITING);

	BINARIZER_ALPHABET ident, acgt;
	binarizer_alphabet_identity(&ident);
	binarizer_alphabet_acgt(&acgt);

	while (1) {
		char * name = fasta_get_name(f1);
		if (name == NULL) {
			break;
		}

		name[0] = name[0] == '<' ? '>' : '<';
		fasta_put_name(f2, name);
		printf("%s\n", name);

		if (fasta_has_sequence(f1)) {

			BINARIZER b1 = binarizer_open(f1, acgt);
			PREDICTOR_DATA data;
			predictor_init_data(&data, PREDICTOR_LENGTH, 0);
			binarizer_add_filter(b1, predictor_filter, &data);
			BINARIZER b2 = binarizer_open(f2, ident);

			ARITHMETIC a = arithmetic_open(b2);

			int bit;
			while (1) {
				bit = binarizer_get_bit(b1);
				if (bit == -1) {
					break;
				}
				//printf("bit %d, pos %d\n", c & 1, (c >> 8) & 255);

				arithmetic_encode_bit(a, bit);
			}
			arithmetic_close(a);

			printf("\n");

			binarizer_close(b1);
			predictor_destroy_data(&data);
			binarizer_close(b2);

		} else {
			printf("empty\n");
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

void decode(char * in, char * out) {
	FASTA f1 = fasta_open(in, FASTA_READING);
	FASTA f2 = fasta_open(out, FASTA_WRITING);

	BINARIZER_ALPHABET ident, a0123;
	binarizer_alphabet_identity(&ident);
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

			BINARIZER b1 = binarizer_open(f1, ident);
			BINARIZER b2 = binarizer_open(f2, a0123);
			PREDICTOR_DATA data;
			predictor_init_data(&data, PREDICTOR_LENGTH, 1);
			binarizer_add_filter(b2, predictor_filter, &data);

			ARITHMETIC a = arithmetic_open(b1);

			int bit;
			while (1) {
				bit = arithmetic_decode_bit(a);
				if (bit == -1) {
					break;
				}
				//printf("bit %d, pos %d\n", c & 1, (c >> 8) & 255);

				binarizer_put_bit(b2, bit);
			}
			arithmetic_close(a);

			printf("\n");

			binarizer_close(b1);
			binarizer_close(b2);
			predictor_destroy_data(&data);

		} else {
			printf("empty\n");
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
	printf("# encoding\n");
	encode(argv[1], argv[2]);
	printf("# decoding\n");
	decode(argv[2], argv[3]);
	printf("# finished\n");

	exit(0);
}
