#ifndef FASTA_H_
#define FASTA_H_

#include <stdio.h>

#define RW_MASK 1
#define READING 1
#define WRITING 0

typedef int flags;

struct fasta {
	FILE * file;
	int rwMode; // rw

	int curSeq; // first sequence has 0
	char curNl; // 1 or 2, reading only
	char curSeqType; // either < or >
	off_t curNamePos; // where name starts
	off_t curSeqPos; // where seq starts
	off_t curSeqSize; // length of sequence
	off_t curChar; // position inside sequence
};

typedef struct fasta * FASTA;

FASTA fasta_open(char * filename, int flags);
void fasta_close(FASTA fasta);

char * fasta_get_name(FASTA fasta);
int fasta_seek_name(FASTA fasta, int delta);
void fasta_put_name(FASTA fasta, char * name);

int fasta_get_char(FASTA fasta);
void rewind_sequence(FASTA fasta);
int fasta_has_sequence(FASTA fasta);

void fasta_put_char(FASTA fasta, int c);

size_t fasta_reserve_space(FASTA fasta, int size);
void fasta_read_space(FASTA fasta, size_t position, int size,
		unsigned char * space);
void fasta_write_space(FASTA fasta, size_t position, int size,
		unsigned char * space);

#endif /* FASTA_H_ */
