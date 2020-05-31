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

#include "realtime.h"
#include "version.h"
#include "sink.h"
#include "sink/cbor.h"
#include "sink/io.h"
#include "sink/ogg.h"

/* FIXME: hacks */
#include <string.h>
#define rand32() rand()

#define META_VERSION	0

/* set all the fields common to every packet */
static void
prepare_cbor_buf(uint8_t *buf, size_t *len)
{
	*len = 0;
	cbor_pack_map_start(buf, len);
	cbor_pack_ustring(buf, len, "ts");
	cbor_pack_uint(buf, len, suscan_gettime_unix());
}

static void
finalize_cbor_buf(uint8_t *buf, size_t *len)
{
	cbor_pack_map_end(buf, len);
}

static int
write_global_meta(struct suscan_sink *sink)
{
	const uint64_t granule = suscan_gettime();
	uint8_t buf[512];
	size_t len;
	int ret;

	prepare_cbor_buf(buf, &len);

	cbor_pack_ustring(buf, &len, "version");
	cbor_pack_uint(buf, &len, META_VERSION);

	cbor_pack_ustring(buf, &len, "generator");
	cbor_pack_ustring(buf, &len, "suscan v" SUSCAN_VERSION_STRING);

	finalize_cbor_buf(buf, &len);

	ret = ogg_write_packet(sink->meta_fd, OGG_START | OGG_END,
			       granule, rand32(), 0, buf, len);

	return ret;
}

static int
write_local_meta_header(struct suscan_sink *sink)
{
	const uint64_t granule = suscan_gettime();
	uint8_t buf[512];
	size_t len;
	int ret;

	prepare_cbor_buf(buf, &len);

	cbor_pack_ustring(buf, &len, "iq");
	cbor_pack_map_start(buf, &len);

	cbor_pack_ustring(buf, &len, "uniq");
	cbor_pack_null(buf, &len); /* always external */

	cbor_pack_ustring(buf, &len, "fmt");
	cbor_pack_ustring(buf, &len, "cf32");

	/* TODO: fc */
	/* TODO: fs */

	cbor_pack_map_end(buf, &len);

	finalize_cbor_buf(buf, &len);

	ret = ogg_write_packet(sink->meta_fd, OGG_START, granule,
			       sink->meta_serial, sink->meta_seq++,
			       buf, len);

	return ret;
}

static int
write_local_meta_update(struct suscan_sink *sink, size_t samples)
{
	const uint64_t granule = suscan_gettime();
	uint8_t buf[512];
	size_t len;
	int ret;

	if (sink->meta_fd == -1)
		return 0;

	prepare_cbor_buf(buf, &len);

	cbor_pack_ustring(buf, &len, "iq");
	cbor_pack_map_start(buf, &len);

	cbor_pack_ustring(buf, &len, "samples");
	cbor_pack_uint(buf, &len, samples);

	/* TODO: fc */

	cbor_pack_map_end(buf, &len);

	finalize_cbor_buf(buf, &len);

	ret = ogg_write_packet(sink->meta_fd, OGG_DATA, granule,
			       sink->meta_serial, sink->meta_seq++,
			       buf, len);

	return ret;
}

static int
write_local_meta_footer(struct suscan_sink *sink)
{
	const uint64_t granule = suscan_gettime();
	uint8_t buf[512];
	size_t len;
	int ret;

	prepare_cbor_buf(buf, &len);
	finalize_cbor_buf(buf, &len);

	ret = ogg_write_packet(sink->meta_fd, OGG_END, granule,
			       sink->meta_serial, sink->meta_seq++, buf, len);

	return ret;
}

struct suscan_sink *
suscan_sink_open(const char *data_fname, const char *meta_fname)
{
	struct suscan_sink *sink;

	sink = malloc(sizeof(struct suscan_sink));
	if (!sink)
		return NULL;

	sink->meta_serial = rand32();
	sink->meta_seq = 0;

	sink->data_fd = creat(data_fname, 0600);
	if (sink->data_fd == -1)
		goto err;

	if (meta_fname) {
		sink->meta_fd = creat(meta_fname, 0600);
		if (sink->meta_fd == -1)
			goto err_close1;

		if (write_global_meta(sink))
			goto err_close2;

		if (write_local_meta_header(sink))
			goto err_close2;
	} else {
		sink->meta_fd = -1;
	}

	return sink;

err_close2:
	close(sink->meta_fd);
	unlink(meta_fname);

err_close1:
	close(sink->data_fd);
	unlink(data_fname);

err:
	free(sink);

	return NULL;
}

void
suscan_sink_close(struct suscan_sink *sink)
{
	close(sink->data_fd);
	if (sink->meta_fd != -1) {
		write_local_meta_footer(sink);
		close(sink->meta_fd);
	}

	free(sink);
}

ssize_t
suscan_sink_write(struct suscan_sink *sink, const SUCOMPLEX *data, size_t len)
{
	ssize_t ret;

	ret = write(sink->data_fd, data, len * sizeof(*data));
	if (ret < 0)
		return 0;

	write_local_meta_update(sink, ret / sizeof(*data));

	return ret / sizeof(*data);
}

#if 0
static int init_global(uint64_t now)
{
	struct nvlist *nvl;
	struct buffer *buf;
	int ret;

	nvl = nvl_alloc();
	VERIFY(nvl != NULL);

	VERIFY0(nvl_set_cstr_dup(nvl, "generator", "filize"));
	VERIFY0(nvl_set_int(nvl, "datetime", now));

	buf = nvl_pack(nvl, VF_CBOR);
	VERIFY(buf != NULL);
	nvl_putref(nvl);

	ret = ogg_write_packet(index_fd, OGG_START | OGG_END, now, rand32(),
			       0, buffer_data(buf), buffer_size(buf));
	buffer_free(buf);

	return ret;
}

static int init_other(uint64_t now)
{
	struct nvlist *nvl;
	struct buffer *buf;
	int ret;

	nvl = nvl_alloc();
	VERIFY(nvl != NULL);

	VERIFY0(nvl_set_cstr_dup(nvl, "type", "application/octet-stream"));
	VERIFY0(nvl_set_null(nvl, "iq-uniq"));

	buf = nvl_pack(nvl, VF_CBOR);
	VERIFY(buf != NULL);
	nvl_putref(nvl);

	ret = ogg_write_packet(index_fd, OGG_START, now, serial, seq++,
			       buffer_data(buf), buffer_size(buf));

	buffer_free(buf);

	return ret;
}

int filize_init(const char *dirname)
{
	const uint64_t now = gettime();
	char fname[1024];
	int ret;

	serial = rand32();

	snprintf(fname, sizeof(fname), "%s/index.ogg", dirname);

	index_fd = xopen(fname, O_WRONLY | O_CREAT, 0600);
	ASSERT3S(index_fd, >=, 0);

	/* global info bitstream - single packet only */
	ret = init_global(now);
	if (ret)
		return ret;

	/* per bitstream info */
	ret = init_other(now);
	if (ret)
		return ret;

	return 0;
}

int filize_deinit(void)
{
	struct nvlist *nvl;
	struct buffer *buf;

	nvl = nvl_alloc();
	VERIFY(nvl != NULL);

	buf = nvl_pack(nvl, VF_CBOR);
	VERIFY(buf != NULL);
	nvl_putref(nvl);

	VERIFY0(ogg_write_packet(index_fd, OGG_END, gettime(), serial, seq++,
				 buffer_data(buf), buffer_size(buf)));
	VERIFY0(xclose(index_fd));

	return 0;
}

int filize_start(int fd)
{
	return 0;
}

int filize_finish(int fd)
{
	raw_file_number++;
	raw_file_offset = 0;

	return 0;
}

int filize_write(int fd, const void *data, size_t len, uint64_t when)
{
	struct nvlist *nvl;
	struct buffer *buf;
	int ret;

	ret = xwrite(fd, data, len);
	if (ret)
		return ret;

	nvl = nvl_alloc();
	VERIFY(nvl != NULL);

	VERIFY0(nvl_set_int(nvl, "datetime", when));
	VERIFY0(nvl_set_int(nvl, "raw-file-idx", raw_file_number));
	VERIFY0(nvl_set_int(nvl, "raw-file-off", raw_file_offset));

	raw_file_offset += len;

	buf = nvl_pack(nvl, VF_CBOR);
	VERIFY(buf != NULL);
	nvl_putref(nvl);

	return ogg_write_packet(index_fd, OGG_DATA, when, serial, seq++,
				buffer_data(buf), buffer_size(buf));
}
#endif
