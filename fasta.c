#include <stdlib.h>

#include "common.h"

#include "fasta.h"

#define LINE_WIDTH (256*256)
#define RESERVED_SPACE 8

#define EOL 10
#define EOL2 13

FASTA_PTR fasta_open(char * filename, int flags) {
	FASTA_PTR fasta = calloc(1, sizeof(*fasta));
	fasta->flags = flags;
	fasta->file = fopen(filename, FASTA_IS_READING(fasta) ? "r" : "w+");

	fasta->curSeq = -1;
	fasta->curNamePos = 0;

	return fasta;
}

void close_sequence(FASTA_PTR fasta) {
	if (FASTA_IS_WRITING(fasta)) {
		if (fasta->curSeqSize > 0) {
			// write size
			if (FASTA_IS_COMPRESSED(fasta)) {
				unsigned char space[RESERVED_SPACE];
				convert_to_data(fasta->curSeqSize, RESERVED_SPACE, space);
				fasta_write_space(fasta, fasta->curSeqPos, RESERVED_SPACE,
						space);
			}

			// append newline
			fputc(EOL, fasta->file);
		}

		// increase counter
		fasta->curSeq++;
		fasta->curSeqPos = 0;
		fasta->curSeqSize = 0;
	}
}

void fasta_close(FASTA_PTR fasta) {
	close_sequence(fasta);

	fclose(fasta->file);
	free(fasta);
}

int get_char(FASTA_PTR fasta) {
	int c;
	if (FASTA_IS_PLAIN(fasta)) {
		do {
			c = fgetc(fasta->file);
		} while (c == EOL || c == EOL2);

		if (c == FASTA_NAME_START_PLAIN || c == FASTA_NAME_START_COMPRESSED
				|| c == EOF) {
			if (c != EOF) {
				fseeko(fasta->file, -1, SEEK_CUR);
			}
			return -1;
		}
	} else {
		off_t pos = ftello(fasta->file);
		int mod;
		do {
			c = fgetc(fasta->file);
			mod = (pos - fasta->curSeqPos) % (LINE_WIDTH + fasta->curNl);
			pos++;
			// skip new lines aligned to LINE_WIDTH + 1 (and +2 if curNl == 2)
		} while (
				mod == LINE_WIDTH + fasta->curNl - 1 || fasta->curNl == 2 ?
						mod == LINE_WIDTH : 0);
	}
	return c;
}

void fasta_fill_info(FASTA_PTR fasta) {
	if (fasta->curSeq == -2) {
		return;
	}

	fasta->curNamePos = ftello(fasta->file);

	int c;

	// store seq type
	c = fgetc(fasta->file);
	if (c != EOF) {
		fasta->curSeqType = c;
	} else {
		fasta->curSeq = -2;
		return;
	}

	// we are at the beginning
	fasta->curChar = 0;

	// find start of the sequence (which is one character past newline)
	fasta->curNl = 1;
	do {
		c = fgetc(fasta->file);
		if (c == EOL2) {
			fasta->curNl = 2;
		}
	} while (c != EOL && c != EOF);
	fasta->curSeqPos = ftello(fasta->file);

	if (c == EOF) {
		fasta->curSeqSize = 0;
		// don't try to read size or test presence of sequence
		return;
	} else {
		fasta->curSeqSize = 1;
	}

	c = fgetc(fasta->file);
	if (c == EOF) {
		fasta->curSeqSize = 0;
		return;
	} else if (c == FASTA_NAME_START_PLAIN || c == FASTA_NAME_START_COMPRESSED) {
		fasta->curSeqSize = 0;
		fseeko(fasta->file, -1, SEEK_CUR);
		return;
	}

	fseeko(fasta->file, -1, SEEK_CUR);
	if (FASTA_IS_PLAIN(fasta)) {
		// do nothing
	} else {
		// try reading rest
		for (int i = 0; i < RESERVED_SPACE; i++) {
			c = get_char(fasta);
			if (c == -1) {
				fasta->curSeqSize = 0;
				return;
			}
		}
		// read actual size
		unsigned char space[RESERVED_SPACE];
		fasta_read_space(fasta, fasta->curSeqPos, RESERVED_SPACE, space);
		fasta->curSeqSize = convert_from_data(RESERVED_SPACE, space);
		fasta->curChar += RESERVED_SPACE;
	}

	if (fasta->curSeq == -1) {
		fasta->curSeq = 0;
	}
}

char * fasta_get_name(FASTA_PTR fasta) {
	if (FASTA_IS_WRITING(fasta)) {
		return NULL;
	}

	if (fasta->curSeq < 0) {
		fasta_fill_info(fasta);
	}
	if (fasta->curSeq < 0) {
		return NULL;
	}

	off_t pos = ftello(fasta->file);

	size_t nameLen = fasta->curSeqPos - fasta->curNamePos;
	char * name = malloc(nameLen + 1); // +1 if file ends with name
	fseeko(fasta->file, fasta->curNamePos, SEEK_SET);
	fread(name, 1, nameLen, fasta->file);

	name[nameLen] = 0;
	if (name[nameLen - 1] == EOL) { // was newline
		name[nameLen - 1] = 0;
	}
	if (name[nameLen - 2] == EOL2) { // was windows newline
		name[nameLen - 2] = 0;
	}

	fseeko(fasta->file, pos, SEEK_SET);

	return name;
}

int fasta_seek_name(FASTA_PTR fasta, int delta) {
	if (FASTA_IS_WRITING(fasta)) {
		return -1;
	}

	if (fasta->curSeq < 0) {
		fasta_fill_info(fasta);
	}
	if (fasta->curSeq < 0) {
		return -1;
	}

	if (delta < 0) {
		// start looking from beginning of the file
		delta = fasta->curSeq + delta;
		fasta->curSeq = 0;
		fseeko(fasta->file, 0, SEEK_SET);
		// fill info about the first seq
		fasta_fill_info(fasta);
	}

	while (delta > 0) {
		if (FASTA_IS_COMPRESSED(fasta)) {
			size_t len = fasta->curSeqSize / 8; // data
			size_t skip = len
			//                          + ceil - 1
					+ (len / LINE_WIDTH + (len % LINE_WIDTH == 0 ? 0 : 1) - 1)
							* fasta->curNl;
			fseeko(fasta->file, fasta->curSeqPos + skip, SEEK_SET);
		}

		int c;
		// and now we may iterate until we find name start or an end
		do {
			c = fgetc(fasta->file);
		} while (c != FASTA_NAME_START_PLAIN && c != FASTA_NAME_START_COMPRESSED
				&& c != EOF);
		if (c != EOF) {
			fseek(fasta->file, -1, SEEK_CUR);
		} else {
			// end of file
			break;
		}

		// now we are at name start
		fasta_fill_info(fasta);
		fasta->curSeq++;
		delta--;
	}

	return fasta->curSeq;
}

void fasta_put_name(FASTA_PTR fasta, char * name) {
	if (FASTA_IS_READING(fasta)) {
		return;
	}

	close_sequence(fasta);

	fasta->curNamePos = ftello(fasta->file);
	fputs(name, fasta->file);
	fputc(EOL, fasta->file);

	fasta->curSeqType = name[0];
	fasta->curSeqPos = ftello(fasta->file);
	fasta->curSeqSize = 0;
}

int fasta_has_sequence(FASTA_PTR fasta) {
	if (FASTA_IS_WRITING(fasta)) {
		return 0;
	}

	if (fasta->curSeq < 0) {
		fasta_fill_info(fasta);
	}
	if (fasta->curSeq < 0) {
		return 0;
	}

	return fasta->curSeqSize > 0;
}

void fasta_rewind(FASTA_PTR fasta) {
	if (FASTA_IS_WRITING(fasta)) {
		return;
	}

	if (fasta->curSeq < 0) {
		fasta_fill_info(fasta);
	}
	if (fasta->curSeq < 0) {
		return;
	}

	fasta->curChar = FASTA_IS_WRITING(fasta) ? RESERVED_SPACE : 0;
	fseeko(fasta->file, fasta->curSeqPos + fasta->curChar, SEEK_SET);
}

int fasta_get_char(FASTA_PTR fasta) {
	if (FASTA_IS_WRITING(fasta)) {
		return -1;
	}

	if (fasta->curSeq < 0) {
		fasta_fill_info(fasta);
	}
	if (fasta->curSeq < 0) {
		return -1;
	}

	int c;
	if (FASTA_IS_PLAIN(fasta)) {
		if (fasta->curSeqSize == 2) {
			return -1;
		}

		c = get_char(fasta);
		if (c == -1) {
			fasta->curSeqSize = 2;
			return -1;
		}
	} else {
		if (fasta->curChar * 8 >= fasta->curSeqSize) {
			return -1;
		}
		c = get_char(fasta);
	}

	fasta->curChar++;

	int len = 0;
	if (FASTA_IS_COMPRESSED(fasta) && fasta->curChar * 8 > fasta->curSeqSize) {
		// this is the last char and it is not complete (strict inequality)
		len = fasta->curSeqSize - fasta->curChar * 8 + 8;
	}

	return c | (len << 8);
}

void put_char(FASTA_PTR fasta, int c) {
	off_t pos = ftello(fasta->file);
	// it is time for a new line
	if ((pos - fasta->curSeqPos) % (LINE_WIDTH + 1) == LINE_WIDTH) {
		fputc(EOL, fasta->file);
	}
	fputc(c, fasta->file);
}

void fasta_put_char(FASTA_PTR fasta, int c) {
	if (FASTA_IS_READING(fasta)) {
		return;
	}

	// reserve if this is the first char of compressed
	if (FASTA_IS_COMPRESSED(fasta) && fasta->curSeqSize == 0) {
		// reserve space for fasta purposes
		// fake size to prevent infinite recursion
		fasta->curSeqSize++;
		fasta_reserve_space(fasta, RESERVED_SPACE);
		fasta->curSeqSize--;
	}

	put_char(fasta, c);

	int len = (c >> 8) & 255;
	if (len == 0) {
		len = 8;
	}
	fasta->curSeqSize += len;
}

off_t fasta_reserve_space(FASTA_PTR fasta, int size) {
	if (FASTA_IS_PLAIN(fasta)) {
		return -1;
	}

	// reserve space could be called before put char
	if (FASTA_IS_WRITING(fasta) && FASTA_IS_COMPRESSED(fasta)
			&& fasta->curSeqSize == 0) {
		// reserve space for fasta purposes
		// fake size to prevent infinite recursion
		fasta->curSeqSize++;
		fasta_reserve_space(fasta, RESERVED_SPACE);
		fasta->curSeqSize--;
	}

	off_t pos = ftello(fasta->file);
	if (FASTA_IS_WRITING(fasta)) {
		for (int i = 0; i < size; i++) {
			put_char(fasta, 255);
		}
		fasta->curSeqSize += size * 8;
	} else {
		// do not attempt to seek because of new lines
		for (int i = 0; i < size; i++) {
			get_char(fasta);
		}
		fasta->curChar += size;
	}
	return pos;
}

void fasta_read_space(FASTA_PTR fasta, off_t position, int size,
		unsigned char * space) {
	if (FASTA_IS_WRITING(fasta) || FASTA_IS_PLAIN(fasta)) {
		return;
	}

	off_t pos = ftello(fasta->file);
	fseeko(fasta->file, position, SEEK_SET);
	for (int i = 0; i < size; i++) {
		space[i] = get_char(fasta);
	}
	fseeko(fasta->file, pos, SEEK_SET);
}

void fasta_write_space(FASTA_PTR fasta, off_t position, int size,
		unsigned char * space) {
	if (FASTA_IS_READING(fasta) || FASTA_IS_PLAIN(fasta)) {
		return;
	}

	off_t pos = ftello(fasta->file);
	fseeko(fasta->file, position, SEEK_SET);
	for (int i = 0; i < size; i++) {
		put_char(fasta, space[i]);
	}
	fseeko(fasta->file, pos, SEEK_SET);
}
