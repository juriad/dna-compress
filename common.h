#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

#define COMPRESS 1
#define DECOMPRESS 0

void convert_to_data(uint64_t value, int size, unsigned char * data);

uint64_t convert_from_data(int size, unsigned char * data);

#endif /* COMMON_H_ */
