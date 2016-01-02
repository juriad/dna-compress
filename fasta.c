#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fasta.h"

#define LINE_WIDTH 80

#define EOL 10
#define EOL2 13

#define NAME_START '>'
#define NAME_START_COMPRESSED '<'

FASTA fasta_open(char * filename, int rwMode) {
	FASTA fasta = calloc(1, sizeof(*fasta));
	fasta->rwMode = rwMode;
	fasta->file = fopen(filename, rwMode == READING ? "r" : "w+");

	fasta->curSeq = -1;
	fasta->curNamePos = 0;

	return fasta;
}

size_t read_size_t(FASTA fasta) {
	uint64_t size = 0;
	for (int i = 7; i >= 0; i--) {
		size += ((u_int64_t) fgetc(fasta->file)) << (i * 8);
	}
	return size;
}

void write_size_t(FASTA fasta, size_t size) {
	for (int i = 7; i >= 0; i--) {
		fputc((((uint64_t) size) >> (i * 8)) & 255, fasta->file);
	}
}

void close_sequence(FASTA fasta) {
	if (fasta->rwMode == WRITING) {
		if (fasta->curSeqSize > 0) {
			// write size
			if (fasta->curSeqType == NAME_START_COMPRESSED) {
				fseek(fasta->file, fasta->curSeqPos, SEEK_SET);
				write_size_t(fasta, fasta->curSeqSize);
				fseek(fasta->file, 0, SEEK_END);
			}

			// append newline
			fputc(EOL, fasta->file);
		}

		// increase counter
		fasta->curSeq++;
		fasta->curNamePos = ftell(fasta->file);
		fasta->curSeqPos = 0;
		fasta->curSeqSize = 0;
	}
}

void fasta_close(FASTA fasta) {
	close_sequence(fasta);

	fclose(fasta->file);
	free(fasta);
}

char * fasta_get_name(FASTA fasta) {
	if (fasta->curSeq < 0) {
		fasta_seek_name(fasta, -1);
		if (fasta->curSeq < 0) {
			return NULL;
		}
	}

	size_t nameLen = fasta->curSeqPos - fasta->curNamePos;
	char * name = malloc(nameLen + 1);
	fseek(fasta->file, fasta->curNamePos, SEEK_SET);
	fread(name, 1, nameLen, fasta->file);

	name[nameLen] = 0;
	if (name[nameLen - 1] == EOL) { // was newline
		name[nameLen - 1] = 0;
	}
	if (name[nameLen - 2] == EOL2) { // was windows newline
		name[nameLen - 2] = 0;
	}

	return name;
}

int fasta_seek_name(FASTA fasta, int delta) {
	int c;

	// only when init
	if (fasta->curSeq < 0) {
		c = fgetc(fasta->file);
		fasta->curSeqType = c;
		if (c != EOF) {
			fseek(fasta->file, -1, SEEK_CUR);
		}
		if (c == EOF || (c != NAME_START && c != NAME_START_COMPRESSED)) {
			return -1;
		}
	}

	if (delta < 0) {
		delta = fasta->curSeq + delta;
		fasta->curSeq = 0;
		fasta->curNamePos = 0;
		fseek(fasta->file, 0, SEEK_SET);
	}

	// not sure
	fseek(fasta->file, fasta->curNamePos, SEEK_SET);

	// searching for name start
	while (delta > 0) {
		// go through the name
		char type = (char) fgetc(fasta->file);

		do {
			c = fgetc(fasta->file);
		} while (c != EOL && c != EOF);

		// we can also expect an empty sequence (determine by the first character)
		// and also an end
		c = fgetc(fasta->file);
		if (c == NAME_START || c == NAME_START_COMPRESSED) {
			// we have found another name
			// do not skip through the sequence
			fseek(fasta->file, -1, SEEK_CUR);
		} else if (type == NAME_START) {
			// go through an uncompressed sequence until we find
			// either an end or a new name
			do {
				c = fgetc(fasta->file);
			} while (c != NAME_START && c != NAME_START_COMPRESSED && c != EOF);
			if (c != EOF) {
				fseek(fasta->file, -1, SEEK_CUR);
			}
		} else if (type == NAME_START_COMPRESSED) {
			// simply skip the promised number of bytes

			// size of each newline
			fseek(fasta->file, -1, SEEK_CUR);

			size_t len = 8 + read_size_t(fasta) / 8; // data
			size_t skip = len - 8
					+ (len / LINE_WIDTH + (len % LINE_WIDTH == 0 ? 0 : 1) - 1)
							* fasta->curNl;
			fseek(fasta->file, skip, SEEK_CUR);

			// and now we may iterate until we find name start or an end
			do {
				c = fgetc(fasta->file);
			} while (c != NAME_START && c != NAME_START_COMPRESSED && c != EOF);
			if (c != EOF) {
				fseek(fasta->file, -1, SEEK_CUR);
			}
		}

		// now fasta->curNamePos is either at name start of past the end of file
		if (c == EOF) {
			// end
			break;
		}

		fasta->curNamePos = ftell(fasta->file);
		fasta->curSeqType = c;
		delta--;
		fasta->curSeq++;
	}

	// find start of the sequence (which is one character past newline)
	fasta->curNl = 1;
	do {
		c = fgetc(fasta->file);
		if (c == EOL2) {
			fasta->curNl = 2;
		}
	} while (c != EOL && c != EOF);
	fasta->curSeqPos = ftell(fasta->file);
	fasta->curChar = 0;

	return fasta->curSeq;
}

void fasta_put_name(FASTA fasta, char * name) {
	close_sequence(fasta);

	fputs(name, fasta->file);
	fputc(EOL, fasta->file);

	fasta->curSeqType = name[0];
	fasta->curSeqPos = ftell(fasta->file);
}

int fasta_get_char(FASTA fasta) {
	int cc;
	int len = 0;
	if (fasta->curSeqType == NAME_START) {
		do {
			cc = fgetc(fasta->file);
		} while (cc == EOL || cc == EOL2);

		if (cc == NAME_START || cc == NAME_START_COMPRESSED || cc == EOF) {
			if (cc != EOF) {
				fseek(fasta->file, -1, SEEK_CUR);
			}
			return -1;
		}
	} else {
		size_t pos = ftell(fasta->file);
		if (pos == fasta->curSeqPos) {
			cc = fgetc(fasta->file);
			if (cc != EOF) {
				fseek(fasta->file, -1, SEEK_CUR);
			}
			if (cc == NAME_START || cc == NAME_START_COMPRESSED || cc == EOF) {
				return -1;
			}
			fasta->curSeqSize = read_size_t(fasta);
			pos += 8;
		}
		if (fasta->curChar * 8 >= fasta->curSeqSize) {
			return -1;
		}
		int mod;
		do {
			cc = fgetc(fasta->file);
			mod = (pos - fasta->curSeqPos) % (LINE_WIDTH + fasta->curNl);
			pos++;
		} while (
				mod == LINE_WIDTH + fasta->curNl - 1 || fasta->curNl == 2 ?
						mod == LINE_WIDTH : 0);
	}

	fasta->curChar++;
	if (fasta->curSeqType == NAME_START_COMPRESSED
			&& fasta->curChar * 8 > fasta->curSeqSize) {
		// this is the last char and it is not complete (strict inequality)
		// set len
		len = fasta->curSeqSize - fasta->curChar * 8 + 8;
	}

	return cc | (len << 8);
}

void fasta_put_char(FASTA fasta, int c) {
	if (fasta->curSeqType == NAME_START_COMPRESSED && fasta->curSeqSize == 0) {
		for (int i = 0; i < 8; i++) {
			fputc(255, fasta->file);
		}
	}

	size_t pos = ftell(fasta->file);
	if ((pos - fasta->curSeqPos) % (LINE_WIDTH + 1) == LINE_WIDTH) {
		fputc(EOL, fasta->file);
	}
	fputc(c, fasta->file);

	int len = (c >> 8) & 255;
	if (len == 0) {
		len = 8;
	}
	fasta->curSeqSize += len;
}
