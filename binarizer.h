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

#define binarizer_symbol(alphabet, sym, bi) (alphabet).bits[sym] = (bi)

struct binarizer {
	BINARIZER_ALPHABET alphabet;
	FASTA fasta;

	unsigned char bits;
	signed char position;
	char length;
};

typedef struct binarizer * BINARIZER;

BINARIZER binarizer_open(FASTA fasta, BINARIZER_ALPHABET alphabet);
void binarizer_close(BINARIZER binarizer);

int binarizer_get_bit(BINARIZER binarizer);
void binarizer_put_bit(BINARIZER binarizer, int bit);

#endif /* BINARIZER_H_ */
