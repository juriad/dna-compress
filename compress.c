#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "fasta.h"
#include "binarizer.h"

void encode(char * in, char * out) {
	FASTA f1 = fasta_open(in, READING);
	FASTA f2 = fasta_open(out, WRITING);

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

		BINARIZER b1 = binarizer_open(f1, acgt);
		BINARIZER b2 = binarizer_open(f2, ident);

		int c;
		while (1) {
			c = binarizer_get_bit(b1);
			if (c == -1) {
				break;
			}
			//printf("bit %d, pos %d\n", c & 1, (c >> 8) & 255);
			binarizer_put_bit(b2, c);
		}
		printf("\n");

		binarizer_close(b1);
		binarizer_close(b2);

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
	FASTA f1 = fasta_open(in, READING);
	FASTA f2 = fasta_open(out, WRITING);

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

		BINARIZER b1 = binarizer_open(f1, ident);
		BINARIZER b2 = binarizer_open(f2, a0123);

		int c;
		while (1) {
			c = binarizer_get_bit(b1);
			if (c == -1) {
				break;
			}
			//printf("bit %d, pos %d\n", c & 1, (c >> 8) & 255);
			binarizer_put_bit(b2, c);
		}
		printf("\n");

		binarizer_close(b1);
		binarizer_close(b2);

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
