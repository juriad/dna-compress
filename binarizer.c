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
		BINARIZER_SYMBOL(*alphabet, i, i);
	}
}

void binarizer_alphabet_acgt(BINARIZER_ALPHABET * alphabet) {
	binarizer_alphabet_init(alphabet, 2);
	BINARIZER_SYMBOL(*alphabet, 'A', 0);
	BINARIZER_SYMBOL(*alphabet, 'a', 0);
	BINARIZER_SYMBOL(*alphabet, 'C', 1);
	BINARIZER_SYMBOL(*alphabet, 'c', 1);
	BINARIZER_SYMBOL(*alphabet, 'G', 2);
	BINARIZER_SYMBOL(*alphabet, 'g', 2);
	BINARIZER_SYMBOL(*alphabet, 'T', 3);
	BINARIZER_SYMBOL(*alphabet, 't', 3);
}

void binarizer_alphabet_0123(BINARIZER_ALPHABET * alphabet) {
	binarizer_alphabet_init(alphabet, 2);
	BINARIZER_SYMBOL(*alphabet, 0, 'A');
	BINARIZER_SYMBOL(*alphabet, 1, 'C');
	BINARIZER_SYMBOL(*alphabet, 2, 'G');
	BINARIZER_SYMBOL(*alphabet, 3, 'T');
}

BINARIZER_PTR binarizer_open(FASTA_PTR fasta, BINARIZER_ALPHABET alphabet) {
	BINARIZER_PTR binarizer = calloc(1, sizeof(*binarizer));
	binarizer->alphabet = alphabet;
	binarizer->fasta = fasta;
	return binarizer;
}

void binarizer_close(BINARIZER_PTR binarizer) {
	if (binarizer->position != 0 && FASTA_IS_WRITING(binarizer->fasta)) {
		fasta_put_char(binarizer->fasta,
				binarizer->alphabet.bits[binarizer->bits]
						| binarizer->position << 8);
	}

	while (binarizer->filters != NULL) {
		struct binarizer_filters * filters = binarizer->filters;
		binarizer->filters = filters->next;
		free(filters);
	}
	free(binarizer);
}

int process_filters(struct binarizer_filters * filters, int bit) {
	if (filters != NULL) {
		bit = process_filters(filters->next, bit);
		bit = filters->filter(bit, filters->data);
	}
	return bit;
}

int binarizer_get_bit(BINARIZER_PTR binarizer) {
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

	return process_filters(binarizer->filters, bit | pos << 8) & 1;
}

void binarizer_put_bit(BINARIZER_PTR binarizer, int bit) {
	if (bit < 0) {
		return;
	}
	bit &= 1;

	int pos = binarizer->position;
	bit = process_filters(binarizer->filters, bit | pos << 8);

	int shift = binarizer->alphabet.bits_length - pos - 1;
	binarizer->bits |= bit << shift;
	binarizer->position = (pos + 1) % binarizer->alphabet.bits_length;

	if (binarizer->position == 0) {
		fasta_put_char(binarizer->fasta,
				binarizer->alphabet.bits[binarizer->bits]);
		binarizer->bits = 0;
	}
}

void binarizer_add_filter(BINARIZER_PTR binarizer, BINARIZER_FILTER filter,
		void * data) {
	struct binarizer_filters * filters = malloc(sizeof(binarizer->filters));
	filters->filter = filter;
	filters->data = data;
	filters->next = binarizer->filters;
	binarizer->filters = filters;
}
