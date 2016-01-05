#include "common.h"

void convert_to_data(uint64_t value, int size, unsigned char * data) {
	for (int i = 0; i < size; i++) {
		data[i] = 0;
		data[i] = value >> ((size - i - 1) * 8) & 255;
	}
}

uint64_t convert_from_data(int size, unsigned char * data) {
	uint64_t value = 0;
	for (int i = 0; i < size; i++) {
		value |= data[i] << ((size - i - 1) * 8);
	}
	return value;
}
