#ifndef FASTA_H_
#define FASTA_H_

#include <stdio.h>

#define FASTA_RW_MASK 1
#define FASTA_READING 1
#define FASTA_WRITING 0

#define FASTA_NAME_START_PLAIN '>'
#define FASTA_NAME_START_COMPRESSED '<'

struct fasta {
	FILE * file;
	int flags; // ...|rw

	int curSeq; // first sequence has 0; -1 = uninitialized; -2 no sequence
	char curNl; // 1 or 2, reading only
	char curSeqType; // either < or >
	off_t curNamePos; // where name starts
	off_t curSeqPos; // where seq starts
	off_t curSeqSize; // length of sequence
	off_t curChar; // position inside sequence
};

typedef struct fasta * FASTA_PTR;

#define FASTA_IS_WRITING(fasta) (((fasta)->flags & FASTA_RW_MASK) == FASTA_WRITING)
#define FASTA_IS_READING(fasta) (!FASTA_IS_WRITING(fasta))

#define FASTA_IS_PLAIN(fasta) ((fasta)->curSeqType == FASTA_NAME_START_PLAIN)
#define FASTA_IS_COMPRESSED(fasta) ((fasta)->curSeqType == FASTA_NAME_START_COMPRESSED)

FASTA_PTR fasta_open(char * filename, int flags);
void fasta_close(FASTA_PTR fasta);

char * fasta_get_name(FASTA_PTR fasta);
int fasta_seek_name(FASTA_PTR fasta, int delta);
void fasta_put_name(FASTA_PTR fasta, char * name);

int fasta_has_sequence(FASTA_PTR fasta);
void fasta_rewind(FASTA_PTR fasta);
int fasta_get_char(FASTA_PTR fasta);
void fasta_put_char(FASTA_PTR fasta, int c);

off_t fasta_reserve_space(FASTA_PTR fasta, int size);
void fasta_read_space(FASTA_PTR fasta, off_t position, int size,
		unsigned char * space);
void fasta_write_space(FASTA_PTR fasta, off_t position, int size,
		unsigned char * space);

#endif /* FASTA_H_ */
