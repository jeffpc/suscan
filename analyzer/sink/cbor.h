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

#ifndef _CBOR_H
#define _CBOR_H

extern void cbor_pack_null(uint8_t *buf, size_t *_len);
extern void cbor_pack_uint(uint8_t *buf, size_t *_len, uint64_t v);
extern void cbor_pack_bstring(uint8_t *buf, size_t *_len, const char *str);
extern void cbor_pack_ustring(uint8_t *buf, size_t *_len, const char *str);
extern void cbor_pack_array_start(uint8_t *buf, size_t *_len);
extern void cbor_pack_array_end(uint8_t *buf, size_t *_len);
extern void cbor_pack_map_start(uint8_t *buf, size_t *_len);
extern void cbor_pack_map_end(uint8_t *buf, size_t *_len);

#endif
