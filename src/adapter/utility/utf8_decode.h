/* utf8_decode.h */
#ifndef UTF8_DECODE_H
#define UTF8_DECODE_H

#define UTF8_END   -1
#define UTF8_ERROR -2
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct json_utf8_decode
{
    int the_index;
    char *the_input;
    int the_length;
    int the_char;
    int the_byte;
} json_utf8_decode;

extern int  utf8_decode_at_byte(json_utf8_decode *utf8);
extern int  utf8_decode_at_character(json_utf8_decode *utf8);
extern void utf8_decode_init(json_utf8_decode *utf8, char p[], int length);
extern int  utf8_decode_next(json_utf8_decode *utf8);

#ifdef __cplusplus
}
#endif

#endif
