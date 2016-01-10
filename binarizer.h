#ifndef BINARIZER_H_
#define BINARIZER_H_

#include "fasta.h"

struct binarizer_alphabet {
	unsigned char bits[256];
	char bits_length;
};

typedef struct binarizer_alphabet BINARIZER_ALPHABET;

void binarizer_alphabet_init(BINARIZER_ALPHABET * alphabet, char bits_length);
void binarizer_alphabet_identity(BINARIZER_ALPHABET * alphabet);
void binarizer_alphabet_acgt(BINARIZER_ALPHABET * alphabet);
void binarizer_alphabet_0123(BINARIZER_ALPHABET * alphabet);

#define BINARIZER_SYMBOL(alphabet, sym, bi) (alphabet).bits[sym] = (bi)

typedef int (*BINARIZER_FILTER)(int, void *);

struct binarizer_filters {
	BINARIZER_FILTER filter;
	void * data;
	struct binarizer_filters * next;
};

struct binarizer {
	FASTA_PTR fasta;
	BINARIZER_ALPHABET alphabet;
	struct binarizer_filters * filters;

	unsigned char bits;
	signed char position;
	char length;
};

typedef struct binarizer * BINARIZER_PTR;

BINARIZER_PTR binarizer_open(FASTA_PTR fasta, BINARIZER_ALPHABET alphabet);
void binarizer_close(BINARIZER_PTR binarizer);

int binarizer_get_bit(BINARIZER_PTR binarizer);
void binarizer_put_bit(BINARIZER_PTR binarizer, int bit);

void binarizer_add_filter(BINARIZER_PTR binarizer, BINARIZER_FILTER filter,
		void * data);

#endif /* BINARIZER_H_ */
