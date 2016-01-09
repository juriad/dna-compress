#ifndef ARITHMETIC_H_
#define ARITHMETIC_H_

#include <stdint.h>

#include "binarizer.h"

typedef uint64_t arithmetic_type;
#define ARITHMETIC_TYPE_FORMAT "%d"//"%02hhX"

typedef arithmetic_type (*ARITHMETIC_MODEL_RANK)(int, arithmetic_type, void *);
typedef int (*ARITHMETIC_MODEL_SELECT)(arithmetic_type, arithmetic_type, void *);
typedef void (*ARITHMETIC_MODEL_UPDATE)(int, void *);

struct arithmetic_model {
	ARITHMETIC_MODEL_RANK model_rank;
	ARITHMETIC_MODEL_SELECT model_select;
	ARITHMETIC_MODEL_UPDATE model_update;
	void * model_data;
};

typedef struct arithmetic_model ARITHMETIC_MODEL;

struct arithmetic {
	BINARIZER binarizer;
	off_t position;
	uint64_t symbols;

	ARITHMETIC_MODEL model;

	arithmetic_type lower;
	arithmetic_type range;
	arithmetic_type pending;
	arithmetic_type zeros;

	char bits;
	arithmetic_type b1;
	arithmetic_type b2;
};

typedef struct arithmetic * ARITHMETIC;

ARITHMETIC arithmetic_open(BINARIZER binarizer, ARITHMETIC_MODEL model);
void arithmetic_close(ARITHMETIC arithmetic);

void arithmetic_encode_symbol(ARITHMETIC arithmetic, int symbol);
int arithmetic_decode_symbol(ARITHMETIC arithmetic);

void arithmetic_set_model(struct arithmetic_model model);

#endif /* ARITHMETIC_H_ */
