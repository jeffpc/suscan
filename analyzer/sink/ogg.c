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

/*
 * RFC 3533: The Ogg Encapsulation Format Version 0
 */

#include "sigutils/types.h"
#include <stdbool.h>

/* FIXME: hacks */
#define VERIFY3U(a, b, c) do { } while (0)
#define STATIC_ASSERT(c)
static inline uint32_t cpu32_to_be(uint32_t in)
{
        return ((in & 0xff000000) >> 24) |
               ((in & 0x00ff0000) >> 8) |
               ((in & 0x0000ff00) << 8) |
               ((in & 0x000000ff) << 24);
}
#define cpu32_to_le(x) ((uint32_t)(x))
#define cpu64_to_le(x) ((uint64_t)(x))

#include "ogg.h"
#include "io.h"

#define PAGE_PATTERN	0x4f676753 /* "OggS" */

struct page {
	uint32_t pattern;
	uint8_t version;
	uint8_t type;
	uint64_t granule; /* ns tick counter */
	uint32_t serial; /* session */
	uint32_t seq; /* received msg number */
	uint32_t cksum;
	uint8_t segments;
} __attribute__((packed,aligned(8)));
#define PAGE_FIXED_SIZE 27 /* size of the above header */

#define PAGE_TYPE_CONT	0x01
#define PAGE_TYPE_BOS	0x02
#define PAGE_TYPE_EOS	0x04

static const uint8_t segment_template[255] = {
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255,
};

static void crc32(const void *raw, size_t n_bytes, uint32_t* crc)
{
	const uint8_t *data = raw;
	static uint32_t table[0x100];

	for (size_t i = 0; i < 256; i++) {
		size_t j, c;

		for (c = i << 24, j = 8; j > 0; --j)
			c = c & 0x80000000 ? (c << 1) ^ 0x04c11db7 : (c << 1);
		table[i] = c;
	}


	for (size_t i = 0; i < n_bytes; i++)
		*crc = (*crc << 8) ^ table[((*crc >> 24) & 0xFF) ^ data[i]];
}

static int ogg_write_page(int fd, uint8_t type, int64_t granule,
			  uint32_t serial, uint32_t seq, const void *buf,
			  size_t len)
{
	struct page page;
	uint8_t fullsegs;
	uint8_t partseg;
	uint32_t crc;
	int ret;

	/* guarantees tha we fit into one ogg page */
	VERIFY3U(len, <=, 255 * 255);

	fullsegs = len / 255;
	partseg = len % 255;

	page.pattern = cpu32_to_be(PAGE_PATTERN); /* BE because it's a string */
	page.version = 0;
	page.type = type;
	page.granule = cpu64_to_le((uint64_t) granule);
	page.serial = cpu32_to_le(serial);
	page.seq = cpu32_to_le(seq);
	page.cksum = 0;
	page.segments = fullsegs + (partseg ? 1 : 0);

	/* checksum */
	crc = 0;
	crc32(&page, PAGE_FIXED_SIZE, &crc);
	crc32(segment_template, fullsegs, &crc);
	if (partseg || (fullsegs && (fullsegs < 255)))
		crc32(&partseg, sizeof(partseg), &crc);
	crc32(buf, len, &crc);
	page.cksum = cpu32_to_le(crc);

	ret = xwrite(fd, &page, PAGE_FIXED_SIZE);
	if (ret)
		return ret;

	if (fullsegs) {
		ret = xwrite(fd, segment_template, fullsegs);
		if (ret)
			return ret;
	}

	/*
	 * write out a partial segement length if:
	 *
	 *  - we have a partial length
	 *  - we wrote out some full segments, but not filled up the page (a
	 *    full page will get continued by the next page)
	 */
	if (partseg || (fullsegs && (fullsegs < 255))) {
		STATIC_ASSERT(sizeof(partseg) == 1);
		ret = xwrite(fd, &partseg, sizeof(partseg));
		if (ret)
			return ret;
	}

	return xwrite(fd, buf, len);
}

int ogg_write_packet(int fd, enum ogg_what what, int64_t granule,
		     uint32_t serial, uint32_t seq, const void *_buf, size_t len)
{
	const uint8_t *buf = _buf;
	uint8_t type;

	type = 0;
	if (what & OGG_START)
		type |= PAGE_TYPE_BOS;
	if (what & OGG_END)
		type |= PAGE_TYPE_EOS;

	do {
		const size_t this_len = MIN(255 * 255, len);
		int ret;

		ret = ogg_write_page(fd, type, granule, serial, seq, buf,
				     this_len);
		if (ret)
			return ret;

		buf += this_len;
		len -= this_len;

		type |= PAGE_TYPE_CONT;
	} while (len);

	return 0;
}
