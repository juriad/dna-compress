#ifndef FASTA_H_
#define FASTA_H_

#include <stdio.h>

#include "common.h"

struct fasta {
	FILE * file;
	int rwMode;

	int curSeq;
	int curNl;
	size_t curNamePos;
	char curSeqType;
	size_t curSeqPos;
	size_t curSeqSize;
	size_t curChar;
};

typedef struct fasta * FASTA;

FASTA fasta_open(char * filename, int rwMode);
void fasta_close(FASTA fasta);

char * fasta_get_name(FASTA fasta);
int fasta_seek_name(FASTA fasta, int delta);
void fasta_put_name(FASTA fasta, char * name);

int fasta_get_char(FASTA fasta);
void fasta_put_char(FASTA fasta, int c);
int fasta_has_sequence(FASTA fasta);

size_t fasta_reserve_space(FASTA fasta, int size);
void fasta_read_space(FASTA fasta, size_t position, int size,
		unsigned char * space);
void fasta_write_space(FASTA fasta, size_t position, int size,
		unsigned char * space);

#endif /* FASTA_H_ */
