#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

#define COMPRESS 1
#define DECOMPRESS 0

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define RANGE(val,a,b) (MIN(MAX((a), (val)), (b)))

void convert_to_data(uint64_t value, int size, unsigned char * data);
uint64_t convert_from_data(int size, unsigned char * data);

#endif /* COMMON_H_ */
