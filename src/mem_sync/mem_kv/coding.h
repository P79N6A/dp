#ifndef _CODING_H_
#define _CODING_H_
#include <stdint.h>
#include <limits.h>
#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include <limits.h>
#include <string.h>

#if defined(WIN32) || defined(_WIN32)
    #ifndef _LITTLE_ENDIAN
    #define _LITTLE_ENDIAN
    #endif

#elif __APPLE__
    #include <machine/endian.h>
    #if BYTE_ORDER == LITTLE_ENDIAN
        #ifndef _LITTLE_ENDIAN
        #define _LITTLE_ENDIAN
        #endif
    #elif BYTE_ORDER == BIG_ENDIAN
        #ifndef _BIG_ENDIAN
        #define _BIG_ENDIAN
        #endif
    #else
        #error "not supported endian"
    #endif

#elif __ANDROID__
    #include <machine/endian.h>
    #if _BYTE_ORDER == _LITTLE_ENDIAN
        #ifndef _LITTLE_ENDIAN
        #define _LITTLE_ENDIAN
        #endif
        #ifdef _BIG_ENDIAN
        #undef _BIG_ENDIAN
        #endif
    #elif _BYTE_ORDER == _BIG_ENDIAN
        #ifndef _BIG_ENDIAN
        #define _BIG_ENDIAN
        #endif
        #ifdef _LITTLE_ENDIAN
        #undef _LITTLE_ENDIAN
        #endif
    #else
        #error "not supported endian"
    #endif

#elif __linux__
    #include <endian.h>
    #if __BYTE_ORDER == __LITTLE_ENDIAN
        #ifndef _LITTLE_ENDIAN
        #define _LITTLE_ENDIAN
        #endif
    #elif __BYTE_ORDER == __BIG_ENDIAN
        #ifndef _BIG_ENDIAN
        #define _BIG_ENDIAN
        #endif
    #else
        #error "not supported endian"
    #endif

#endif

#define bitswap64(v)    \
    ( (((v) & 0xff00000000000000ULL) >> 56) \
    | (((v) & 0x00ff000000000000ULL) >> 40) \
    | (((v) & 0x0000ff0000000000ULL) >> 24) \
    | (((v) & 0x000000ff00000000ULL) >>  8) \
    | (((v) & 0x00000000ff000000ULL) <<  8) \
    | (((v) & 0x0000000000ff0000ULL) << 24) \
    | (((v) & 0x000000000000ff00ULL) << 40) \
    | (((v) & 0x00000000000000ffULL) << 56) )

#define bitswap32(v)    \
    ( (((v) & 0xff000000) >> 24) \
    | (((v) & 0x00ff0000) >>  8) \
    | (((v) & 0x0000ff00) <<  8) \
    | (((v) & 0x000000ff) << 24) )

#define bitswap16(v)    \
    ( (((v) & 0xff00) >> 8) \
    | (((v) & 0x00ff) << 8) )

#if defined(_LITTLE_ENDIAN)
    // convert to big endian
    #define _enc64(v) bitswap64(v)
    #define _dec64(v) bitswap64(v)
    #define _enc32(v) bitswap32(v)
    #define _dec32(v) bitswap32(v)
    #define _enc16(v) bitswap16(v)
    #define _dec16(v) bitswap16(v)
#else
    // big endian .. do nothing
    #define _enc64(v) (v)
    #define _dec64(v) (v)
    #define _enc32(v) (v)
    #define _dec32(v) (v)
    #define _enc16(v) (v)
    #define _dec16(v) (v)
#endif

#ifdef __GNUC__
#define TYPEOF(x) (__typeof__(x))
#else
#define TYPEOF(x)
#endif

#define bitsizeof(x)  (CHAR_BIT * sizeof(x))
#define MSB(x, bits) ((x) & TYPEOF(x)(~0ULL << (bitsizeof(x) - (bits))))

static inline uintmax_t _decode_varint(char **bufp)
{
    unsigned char *buf = (unsigned char *)(*bufp);
    unsigned char c = *buf++;
    uintmax_t val = c & 127;
    while (c & 128) {
        val += 1;
        if (!val || MSB(val, 7))
            return 0; /* overflow */
        c = *buf++;
        val = (val << 7) + (c & 127);
    }
    *bufp = (char *)buf;
    return val;
}

static inline int _encode_varint(uintmax_t value, unsigned char *buf)
{
    unsigned char varint[16];
    unsigned pos = sizeof(varint) - 1;
    varint[pos] = value & 127;
    while (value >>= 7)
        varint[--pos] = 128 | (--value & 127);
    if (buf)
        memcpy(buf, varint + pos, sizeof(varint) - pos);
    return sizeof(varint) - pos;
}

static inline char *enc_fix16(char *buf, uintmax_t n)
{
    n = _enc16(n);
    memcpy(buf, &n, 2);    

    return buf + 2;
}

static inline char *enc_fix32(char *buf, uintmax_t n)
{
    n = _enc32(n);
    memcpy(buf, &n, 4);    

    return buf + 4;
}

static inline char *enc_fix64(char *buf, uintmax_t n)
{
    n = _enc64(n);
    memcpy(buf, &n, 8);    

    return buf + 8;
}

static inline char *enc_varint(char *buf, uintmax_t n)
{
    int len = _encode_varint(n, (unsigned char *)buf);

    return (buf + len);
}

static inline uint16_t dec_fix16(char *buf, char **dst)
{
    uint16_t v = 0;
    memcpy(&v, buf, 2);
    v = _dec16(v);
    if (dst) *dst = buf + 2;

    return v;
}

static inline uint32_t dec_fix32(char *buf, char **dst)
{
    uint32_t v = 0;
    memcpy(&v, buf, 4);
    v = _dec32(v);
    if (dst) *dst = buf + 4;

    return v;
}

static inline uint64_t dec_fix64(char *buf, char **dst)
{
    uint64_t v = 0;
    memcpy(&v, buf, 8);
    v = _dec64(v);
    if (dst) *dst = buf + 8;

    return v;
}

static inline uintmax_t dec_varint(char *buf, char **dst)
{
    char *bp = buf;
    uintmax_t v = _decode_varint(&bp);
    if (dst) *dst = bp;

    return v;
}

#ifdef __cplusplus
}
#endif

#endif

