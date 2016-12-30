#ifndef _CRC_H_
#define _CRC_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct crc16_s crc16_t;
typedef struct crc32_s crc32_t;

uint16_t calc_crc16(const void *data, size_t sz);

uint32_t calc_crc32(const void *data, size_t sz);

crc16_t*  crc16_start(void);

int crc16_append(crc16_t* crc, const void* data, size_t len);

uint16_t  crc16_finish(crc16_t* crc);

crc32_t*  crc32_start(void);

int crc32_append(crc32_t* crc, const void* data, size_t len);

uint32_t  crc32_finish(crc32_t* crc);

#ifdef __cplusplus
}
#endif

#endif


