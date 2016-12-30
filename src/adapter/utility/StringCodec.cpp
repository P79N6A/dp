#include <cctype>
#include "StringCodec.h"
#include "utf8_to_utf16.h"

using namespace std;

typedef unsigned char u_char;
typedef unsigned int uint32_t;
#define REVERSE16(us) (((us & 0xf) << 12) | (((us >> 4) & 0xf) << 8) | (((us >> 8) & 0xf) << 4) | ((us >> 12) & 0xf))
static const char digits[] = "0123456789abcdef";

static unsigned short crctab[] =    /* CRC lookup table */
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0

};


void StringCodec::UrlEncode(const std::string& src, std::string& dst)
{
    uint32_t       *escape;
    static u_char   hex[] = "0123456789abcdef";

    static uint32_t   form[] =
    {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

        /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0xfc009ffe, /* 1111 1100 0000 0000  1001 1111 1111 1110 */

        /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x78000001, /* 0111 1000 0000 0000  0000 0000 0000 0001 */

        /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0xf8000001, /* 1111 1000 0000 0000  0000 0000 0000 0001 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

    escape = form;

    int size = src.size();
    const u_char* p = (const u_char*)src.data();
    while (size)
    {
        if (escape[*p >> 5] & (1 << (*p & 0x1f)))
        {
            dst.append(1, '%');
            dst.append(1, hex[*p >> 4]);
            dst.append(1, hex[*p & 0xf]);

        }
        else
        {
            if (*p == ' ')
            {
                dst.append(1, '+');
            }
            else
            {
                dst.append(1, *p);
            }
        }
        size--;
        ++p;
    }
    return;
}


void StringCodec::UrlDecode(const string& src, string& dst)
{
    u_char  *s, ch, c, decoded;
    enum
    {
        sw_usual = 0,
        sw_quoted,
        sw_quoted_second
    }state;

    s = (u_char*)src.c_str();
    size_t size = src.size();

    state = sw_usual;
    decoded = 0;

    while (size--)
    {
        ch = *s++;
        switch (state)
        {
            case sw_usual:
                if (ch == '+')
                {
                    dst.append(1, ' ');
                    break;
                }
                if (ch == '%')
                {
                    state = sw_quoted;
                    break;
                }
                dst.append(1, ch);
                break;
            case sw_quoted:
                if (ch >= '0' && ch <= '9')
                {
                    decoded = (u_char) (ch - '0');
                    state = sw_quoted_second;
                    break;
                }
                c = (u_char) (ch | 0x20);
                if (c >= 'a' && c <= 'f')
                {
                    decoded = (u_char) (c - 'a' + 10);
                    state = sw_quoted_second;
                    break;
                }
                /* the invalid quoted character */
                state = sw_usual;
                dst.append(1, '%');
                dst.append(1, ch);
                break;
            case sw_quoted_second:
                state = sw_usual;
                if (ch >= '0' && ch <= '9')
                {
                    ch = (u_char) ((decoded << 4) + ch - '0');
                    dst.append(1, ch);
                    break;
                }
                c = (u_char) (ch | 0x20);
                if (c >= 'a' && c <= 'f')
                {
                    ch = (u_char) ((decoded << 4) + c - 'a' + 10);
                    dst.append(1, ch);
                    break;
                }
                /* the invalid quoted character */
                dst.append(1, '%');
                dst.append(1, *(s-2));
                dst.append(1, *(s-1));
                break;
        }
    }
}

bool StringCodec::Base64Encode(const std::string& src, std::string& dst)
{
    return Base64Encode(src.c_str(), src.length(), dst);
}

bool StringCodec::Base64Encode(const char *src, int src_len, std::string& dst)
{
    static u_char   basis64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = src_len;
    u_char *s;
    s = (u_char*)src;
    while (len > 2)
    {
        dst.append(1, (u_char) basis64[(s[0] >> 2) & 0x3f]);
        dst.append(1, (u_char) basis64[((s[0] & 3) << 4) | (s[1] >> 4)]);
        dst.append(1, (u_char) basis64[((s[1] & 0x0f) << 2) | (s[2] >> 6)]);
        dst.append(1, (u_char) basis64[s[2] & 0x3f]);

        s += 3;
        len -= 3;
    }

    if (len)
    {
        dst.append(1, (u_char) basis64[(s[0] >> 2) & 0x3f]);
        if (len == 1)
        {
            dst.append(1, (u_char) basis64[(s[0] & 3) << 4]);
            dst.append(1, (u_char) '=');
        }
        else
        {
            dst.append(1, (u_char) basis64[((s[0] & 3) << 4) | (s[1] >> 4)]);
            dst.append(1, (u_char) basis64[(s[1] & 0x0f) << 2]);
        }
        dst.append(1, (u_char) '=');
    }
    return true;
}

bool StringCodec::Base64Decode(const string& src, string& dst)
{
    static u_char   basis[] =
    {
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 62, 77, 77, 77, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 77, 77, 77, 77, 77,
        77,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 77, 77, 77, 77, 77,
        77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 77, 77, 77, 77, 77,

        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77
    };
    size_t len;
    u_char *s;

    for (len = 0; len < src.size(); len++)
    {
        if (src[len] == '=')
        {
            break;
        }

        if (basis[(u_char)src[len]] == 77)
        {
            return false;
        }
    }

    if (len % 4 == 1)
    {
        return false;
    }

    s = (u_char*)src.c_str();

    while (len > 3)
    {
        dst.append(1, (u_char) (basis[s[0]] << 2 | basis[s[1]] >> 4));
        dst.append(1, (u_char) (basis[s[1]] << 4 | basis[s[2]] >> 2));
        dst.append(1, (u_char) (basis[s[2]] << 6 | basis[s[3]]));

        s += 4;
        len -= 4;
    }
    if (len > 1)
    {
        dst.append(1, (u_char) (basis[s[0]] << 2 | basis[s[1]] >> 4));
    }

    if (len > 2)
    {
        dst.append(1, (u_char) (basis[s[1]] << 4 | basis[s[2]] >> 2));
    }

    return true;
}

const string StringCodec::Replace(const string& src, const string& dst, char c)
{
    int pos = 0;
    string d = "";
    size_t p = 0;
    while (string::npos != (p = src.find_first_of(dst, pos)))
    {
        if (d.empty())
        {
            d = src;
        }
        d[p] = c;
        pos = p + 1;
    }
    if (d.empty())
    {
        return src;
    }
    return d;
}

bool StringCodec::Replace(string& input, const string& sub, const string& dst)
{
    size_t pos = input.find(sub);
    if (string::npos == pos)
    {
        return false;
    }
    input.replace(pos, sub.length(), dst);
    return true;
}

bool StringCodec::JsonEncode(const std::string& src, std::string& dst)
{
    if ( src.empty() )
    {
        return false;
    }
    int pos = 0;
    unsigned short us;
    unsigned short *utf16;
    static int s_buflen = 0;
    static char* s_buf = NULL;
    int s = sizeof(unsigned short) * src.size();
    if (s_buflen < s)
    {
        if (s_buf)
        {
            delete []s_buf;
        }
        s_buflen = s;
        s_buf = (char*) new char[s_buflen];
    }
    utf16 = (unsigned short*)s_buf;
    int len = utf8_to_utf16(utf16, (char*)src.c_str(), src.size());
    if (len <= 0)
    {
        return false;
    }

    while(pos < len)
    {
        us = utf16[pos++];
        switch (us)
        {
            case '"':
                dst.append("\\\"");
                break;
            case '\\':
                dst.append("\\\\");
                break;
            case '/':
                dst.append("\\/");
                break;
            case '\b':
                dst.append("\\b");
                break;
            case '\f':
                dst.append("\\f");
                break;
            case '\n':
                dst.append("\\n");
                break;
            case '\r':
                dst.append("\\r");
                break;
            case '\t':
                dst.append("\\t");
                break;
            default:
                if (us >= ' ' && (us & 127) == us)
                {
                    dst.append(1, (char)us);
                }
                else
                {
                    dst.append("\\u");
                    us = REVERSE16(us);

                    dst.append(1, digits[us & ((1 << 4) - 1)]);
                    us >>= 4;
                    dst.append(1, digits[us & ((1 << 4) - 1)]);
                    us >>= 4;
                    dst.append(1, digits[us & ((1 << 4) - 1)]);
                    us >>= 4;
                    dst.append(1, digits[us & ((1 << 4) - 1)]);
                }
                break;
        }
    }
    return true;
}

string StringCodec::ToLowercase(const std::string& src)
{
    string lowerCased;
    for (string::const_iterator i = src.begin(); i != src.end(); ++i)
    {
        lowerCased += tolower(*i);
    }
    return lowerCased;
}

unsigned short StringCodec::CRC16(const char* buf, int len)
{
    unsigned short crc = 0;
    int i;
    for (i = 0; i < len; i++)
    {
        crc = crctab[((crc >> 8) ^ *buf++) & 0xFF] ^ (crc << 8);
    }
    return crc;
}
