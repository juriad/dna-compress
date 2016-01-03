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

void close_sequence(FASTA fasta) {
	if (fasta->rwMode == WRITING) {
		if (fasta->curSeqSize > 0) {
			// write size
			if (fasta->curSeqType == NAME_START_COMPRESSED) {
				unsigned char space[8];
				convert_to_data(fasta->curSeqSize, 8, space);
				fasta_write_space(fasta, fasta->curSeqPos, 8, space);
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
		} else if (fasta_has_sequence(fasta)) {
			if (type == NAME_START) {
				// go through an uncompressed sequence until we find
				// either an end or a new name
				do {
					c = fgetc(fasta->file);
				} while (c != NAME_START && c != NAME_START_COMPRESSED
						&& c != EOF);
				if (c != EOF) {
					fseek(fasta->file, -1, SEEK_CUR);
				}
			} else if (type == NAME_START_COMPRESSED) {
				// simply skip the promised number of bytes

				size_t pos = ftell(fasta->file);
				unsigned char space[8];
				fasta_read_space(fasta, pos - 1, 8, space);

				size_t len = convert_from_data(8, space) / 8; // data
				size_t skip = len - 1 // + 1 because we have read one char (c)
						+ (len / LINE_WIDTH + (len % LINE_WIDTH == 0 ? 0 : 1)
								- 1) * fasta->curNl;
				fseek(fasta->file, skip, SEEK_CUR);

				// and now we may iterate until we find name start or an end
				do {
					c = fgetc(fasta->file);
				} while (c != NAME_START && c != NAME_START_COMPRESSED
						&& c != EOF);
				if (c != EOF) {
					fseek(fasta->file, -1, SEEK_CUR);
				}
			}
		}

		// now fasta->curNamePos is either at name start of past the end of file
		if (c == EOF) {
			// end
			break;
		}

		fasta->curNamePos = ftell(fasta->file);
		fasta->curSeqType = c;
		fasta->curSeq++;
		fasta->curSeqSize = 0;
		delta--;
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
	fasta->curSeqSize = 0;
}

int fasta_has_sequence(FASTA fasta) {
	if (fasta->curChar > 0) {
		return 1;
	}

	int ret = 1;
	size_t pos = ftell(fasta->file);

	int c;
	if (fasta->curSeqType == NAME_START) {
		do {
			c = fgetc(fasta->file);
		} while (c == EOL || c == EOL2);
		if (c == NAME_START || c == NAME_START_COMPRESSED || c == EOF) {
			ret = 0;
		}
	} else {
		for (int i = 0; i < 7; i++) {
			c = fgetc(fasta->file);
			if (c == NAME_START || c == NAME_START_COMPRESSED || c == EOF) {
				ret = 0;
				break;
			}
		}
	}

	fseek(fasta->file, pos, SEEK_SET);
	return ret;
}

int fasta_get_char(FASTA fasta) {
	int c;
	int len = 0;
	if (fasta->curSeqType == NAME_START) {
		do {
			c = fgetc(fasta->file);
		} while (c == EOL || c == EOL2);

		if (c == NAME_START || c == NAME_START_COMPRESSED || c == EOF) {
			if (c != EOF) {
				fseek(fasta->file, -1, SEEK_CUR);
			}
			return -1;
		}
		fasta->curChar++;
	} else {
		size_t pos = ftell(fasta->file);
		if (pos == fasta->curSeqPos) {
			c = fgetc(fasta->file);
			if (c != EOF) {
				fseek(fasta->file, -1, SEEK_CUR);
			}
			if (c == NAME_START || c == NAME_START_COMPRESSED || c == EOF) {
				return -1;
			}

			fasta_reserve_space(fasta, 8);
		}
		pos = ftell(fasta->file);
		if (fasta->curChar * 8 >= fasta->curSeqSize) {
			return -1;
		}
		int mod;
		do {
			c = fgetc(fasta->file);
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

	return c | (len << 8);
}

void fasta_put_char(FASTA fasta, int c) {
	if (fasta->curSeqType == NAME_START_COMPRESSED && fasta->curSeqSize == 0) {
		fasta_reserve_space(fasta, 8);
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

size_t fasta_reserve_space(FASTA fasta, int size) {
	// reserve space could be called before put or get char
	if (fasta->curSeqType == NAME_START_COMPRESSED) {
		if ((fasta->rwMode == WRITING && fasta->curSeqSize == 0)
				|| (fasta->rwMode == READING && fasta->curChar == 0)) {
			// reserve space for fasta purposes
			// fake size to prevent infinite recursion
			if (fasta->rwMode == READING) {
				fasta->curChar++;
			} else {
				fasta->curSeqSize++;
			}

			size_t pos = fasta_reserve_space(fasta, 8);

			if (fasta->rwMode == READING) {
				unsigned char space[8];
				fasta_read_space(fasta, pos, 8, space);
				fasta->curSeqSize = convert_from_data(8, space);

				fasta->curChar--;
			} else {
				fasta->curSeqSize--;
			}
		}
	}

	size_t pos = ftell(fasta->file);
	if (fasta->rwMode == WRITING) {
		for (int i = 0; i < size; i++) {
			fputc(255, fasta->file);
		}
		fasta->curSeqSize += size * 8;
	} else {
		fseek(fasta->file, size, SEEK_CUR);
		fasta->curChar += 8;
	}
	return pos;
}

void fasta_read_space(FASTA fasta, size_t position, int size,
		unsigned char * space) {
	size_t pos = ftell(fasta->file);
	fseek(fasta->file, position, SEEK_SET);
	for (int i = 0; i < size; i++) {
		space[i] = fgetc(fasta->file);
	}
	fseek(fasta->file, pos, SEEK_SET);
}

void fasta_write_space(FASTA fasta, size_t position, int size,
		unsigned char * space) {
	size_t pos = ftell(fasta->file);
	fseek(fasta->file, position, SEEK_SET);
	for (int i = 0; i < size; i++) {
		fputc(space[i], fasta->file);
	}
	fseek(fasta->file, pos, SEEK_SET);
}
