/*
 * Copyright (c) 2020 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef _SINK_H
#define _SINK_H

#include "sigutils/types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct suscan_sink {
	int data_fd;
	int meta_fd;

	uint32_t meta_serial;
	uint32_t meta_seq;
};

extern struct suscan_sink *suscan_sink_open(const char *data_fname,
					    const char *meta_fname);
extern void suscan_sink_close(struct suscan_sink *sink);

extern ssize_t suscan_sink_write(struct suscan_sink *sink, const SUCOMPLEX *data,
				 size_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
