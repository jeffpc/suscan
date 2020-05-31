/*
 * Copyright (c) 2017-2020 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * RFC 7049: Concise Binary Object Representation (CBOR)
 */

#include <string.h>
#include "sigutils/types.h"

enum major_type {
        CMT_UINT  = 0,
        CMT_NINT  = 1,
        CMT_BYTE  = 2,
        CMT_TEXT  = 3,
        CMT_ARRAY = 4,
        CMT_MAP   = 5,
        CMT_TAG   = 6,
        CMT_FLOAT = 7,
};

#define ADDL_UINT8              24
#define ADDL_UINT16             25
#define ADDL_UINT32             26
#define ADDL_UINT64             27
#define ADDL_ARRAY_INDEF        31
#define ADDL_MAP_INDEF          31
#define ADDL_FLOAT_FALSE        20
#define ADDL_FLOAT_TRUE         21
#define ADDL_FLOAT_NULL         22
#define ADDL_FLOAT_BREAK        31

static void cbor_pack_type_byte(uint8_t *buf, size_t *_len, uint8_t major, uint8_t addl)
{
	buf[*_len] = (major << 5) | addl;
	*_len += 1;
}

static void cbor_pack_type(uint8_t *buf, size_t *_len, uint8_t major, uint64_t addl)
{
	size_t len = *_len;

	if (addl <= 23) {
		buf[len++] = (major << 5) | addl;
	} else if (addl <= 255) {
		buf[len++] = (major << 5) | ADDL_UINT8;
		buf[len++] = addl;
	} else if (addl <= 0xffff) {
		buf[len++] = (major << 5) | ADDL_UINT16;
		buf[len++] = (addl >> 8) & 0xff;
		buf[len++] = (addl     ) & 0xff;
	} else if (addl <= 0xffffffff) {
		buf[len++] = (major << 5) | ADDL_UINT32;
		buf[len++] = (addl >> 24) & 0xff;
		buf[len++] = (addl >> 16) & 0xff;
		buf[len++] = (addl >>  8) & 0xff;
		buf[len++] = (addl      ) & 0xff;
	} else {
		buf[len++] = (major << 5) | ADDL_UINT64;
		buf[len++] = (addl >> 56) & 0xff;
		buf[len++] = (addl >> 48) & 0xff;
		buf[len++] = (addl >> 40) & 0xff;
		buf[len++] = (addl >> 32) & 0xff;
		buf[len++] = (addl >> 24) & 0xff;
		buf[len++] = (addl >> 16) & 0xff;
		buf[len++] = (addl >>  8) & 0xff;
		buf[len++] = (addl      ) & 0xff;
	}

	*_len = len;
}

void cbor_pack_null(uint8_t *buf, size_t *_len)
{
	cbor_pack_type_byte(buf, _len, CMT_FLOAT, ADDL_FLOAT_NULL);
}

void cbor_pack_uint(uint8_t *buf, size_t *_len, uint64_t v)
{
	cbor_pack_type(buf, _len, CMT_UINT, v);
}

static void cbor_pack_string(uint8_t *buf, size_t *_len, int major, const char *str)
{
	size_t len;

	cbor_pack_type(buf, _len, major, strlen(str));

	len = *_len;

	for (; *str != '\0'; str++)
		buf[len++] = *str;

	*_len = len;
}

void cbor_pack_bstring(uint8_t *buf, size_t *_len, const char *str)
{
	cbor_pack_string(buf, _len, CMT_BYTE, str);
}

void cbor_pack_ustring(uint8_t *buf, size_t *_len, const char *str)
{
	cbor_pack_string(buf, _len, CMT_TEXT, str);
}

void cbor_pack_array_start(uint8_t *buf, size_t *_len)
{
	cbor_pack_type_byte(buf, _len, CMT_ARRAY, ADDL_ARRAY_INDEF);
}

void cbor_pack_array_end(uint8_t *buf, size_t *_len)
{
	cbor_pack_type_byte(buf, _len, CMT_FLOAT, ADDL_FLOAT_BREAK);
}

void cbor_pack_map_start(uint8_t *buf, size_t *_len)
{
	cbor_pack_type_byte(buf, _len, CMT_MAP, ADDL_MAP_INDEF);
}

void cbor_pack_map_end(uint8_t *buf, size_t *_len)
{
	cbor_pack_type_byte(buf, _len, CMT_FLOAT, ADDL_FLOAT_BREAK);
}
