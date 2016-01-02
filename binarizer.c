#include <stdlib.h>

#include "common.h"
#include "fasta.h"

#include "binarizer.h"

void binarizer_alphabet_init(BINARIZER_ALPHABET * alphabet, char bits_length) {
	for (int i = 0; i < 256; i++) {
		alphabet->bits[i] = 0;
	}
	alphabet->bits_length = bits_length;
}

void binarizer_alphabet_identity(BINARIZER_ALPHABET * alphabet) {
	binarizer_alphabet_init(alphabet, 8);
	for (int i = 0; i < 256; i++) {
		binarizer_symbol(*alphabet, i, i);
	}
}

void binarizer_alphabet_acgt(BINARIZER_ALPHABET * alphabet) {
	binarizer_alphabet_init(alphabet, 2);
	binarizer_symbol(*alphabet, 'A', 0);
	binarizer_symbol(*alphabet, 'C', 1);
	binarizer_symbol(*alphabet, 'G', 2);
	binarizer_symbol(*alphabet, 'T', 3);
}

void binarizer_alphabet_0123(BINARIZER_ALPHABET * alphabet) {
	binarizer_alphabet_init(alphabet, 2);
	binarizer_symbol(*alphabet, 0, 'A');
	binarizer_symbol(*alphabet, 1, 'C');
	binarizer_symbol(*alphabet, 2, 'G');
	binarizer_symbol(*alphabet, 3, 'T');
}

BINARIZER binarizer_open(FASTA fasta, BINARIZER_ALPHABET alphabet) {
	BINARIZER binarizer = calloc(1, sizeof(*binarizer));
	binarizer->alphabet = alphabet;
	binarizer->fasta = fasta;
	binarizer->rwMode = fasta->rwMode;
	return binarizer;
}

void binarizer_close(BINARIZER binarizer) {
	if (binarizer->position != 0 && binarizer->rwMode == WRITING) {
		fasta_put_char(binarizer->fasta,
				binarizer->alphabet.bits[binarizer->bits]
						| binarizer->position << 8);
	}
	free(binarizer);
}

int binarizer_get_bit(BINARIZER binarizer) {
	if (binarizer->position < 0) {
		return -1;
	}

	if (binarizer->position == 0) {
		int symbol = fasta_get_char(binarizer->fasta);
		if (symbol < 0) {
			binarizer->position = -1;
			return -1;
		}

		binarizer->bits = binarizer->alphabet.bits[symbol & 255];

		binarizer->length = symbol >> 8 & 255;
		if (binarizer->length == 0) {
			binarizer->length = 8;
		}
	}

	int pos = binarizer->position;
	int shift = binarizer->alphabet.bits_length - pos - 1;
	int bit = (binarizer->bits >> shift) & 1;
	binarizer->position = (pos + 1) % binarizer->alphabet.bits_length;

	if (binarizer->position >= binarizer->length) {
		// only if last char is partial
		binarizer->position = -1;
	}

	return bit | pos << 8;
}

void binarizer_put_bit(BINARIZER binarizer, int bit) {
	if (bit < 0) {
		return;
	}

	int pos = binarizer->position;
	int shift = binarizer->alphabet.bits_length - pos - 1;
	binarizer->bits |= (bit & 1) << shift;
	binarizer->position = (pos + 1) % binarizer->alphabet.bits_length;

	if (binarizer->position == 0) {
		fasta_put_char(binarizer->fasta,
				binarizer->alphabet.bits[binarizer->bits]);
		binarizer->bits = 0;
	}
}
