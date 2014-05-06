/*
 * nghttp2 - HTTP/2 C Library
 *
 * Copyright (c) 2012 Tatsuhiro Tsujikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef NGHTTP2_FRAME_H
#define NGHTTP2_FRAME_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <nghttp2/nghttp2.h>
#include "nghttp2_hd.h"
#include "nghttp2_buf.h"

#define NGHTTP2_FRAME_LENGTH_MASK ((1 << 14) - 1)
#define NGHTTP2_STREAM_ID_MASK ((1u << 31) - 1)
#define NGHTTP2_PRI_GROUP_ID_MASK ((1u << 31) - 1)
#define NGHTTP2_PRIORITY_MASK ((1u << 31) - 1)
#define NGHTTP2_WINDOW_SIZE_INCREMENT_MASK ((1u << 31) - 1)
#define NGHTTP2_SETTINGS_ID_MASK ((1 << 24) - 1)

/* The number of bytes of frame header. */
#define NGHTTP2_FRAME_HDLEN 8

#define NGHTTP2_MAX_PAYLOADLEN 16383
/* The one frame buffer length for tranmission.  We may use several of
   them to support CONTINUATION.  To account for padding specifiers
   (PAD_HIGH and PAD_LOW), we allocate extra 2 bytes, which saves
   extra large memcopying. */
#define NGHTTP2_FRAMEBUF_CHUNKLEN \
  (NGHTTP2_FRAME_HDLEN + 2 + NGHTTP2_MAX_PAYLOADLEN)

/* The maximum length of DATA frame payload. */
#define NGHTTP2_DATA_PAYLOADLEN 4096

/* The number of bytes for each SETTINGS entry */
#define NGHTTP2_FRAME_SETTINGS_ENTRY_LENGTH 5

/* Category of frames. */
typedef enum {
  /* non-DATA frame */
  NGHTTP2_CAT_CTRL,
  /* DATA frame */
  NGHTTP2_CAT_DATA
} nghttp2_frame_category;

/**
 * @struct
 *
 * The DATA frame used in the library privately. It has the following
 * members:
 */
typedef struct {
  nghttp2_frame_hd hd;
  /**
   * The data to be sent for this DATA frame.
   */
  nghttp2_data_provider data_prd;
  /**
   * The number of bytes added as padding. This includes PAD_HIGH and
   * PAD_LOW.
   */
  size_t padlen;
  /**
   * The flag to indicate whether EOF was reached or not. Initially
   * |eof| is 0. It becomes 1 after all data were read. This is used
   * exclusively by nghttp2 library and not in the spec.
   */
  uint8_t eof;
} nghttp2_private_data;

int nghttp2_frame_is_data_frame(uint8_t *head);

void nghttp2_frame_pack_frame_hd(uint8_t *buf, const nghttp2_frame_hd *hd);

void nghttp2_frame_unpack_frame_hd(nghttp2_frame_hd *hd, const uint8_t* buf);

/**
 * Returns the number of priority field depending on the |flags|.  If
 * |flags| has neither NGHTTP2_FLAG_PRIORITY_GROUP nor
 * NGHTTP2_FLAG_PRIORITY_DEPENDENCY set, return 0.
 */
size_t nghttp2_frame_priority_len(uint8_t flags);

/**
 * Packs the |pri_spec| in |buf|.  This function assumes |buf| has
 * enough space for serialization.
 */
void nghttp2_frame_pack_priority_spec(uint8_t *buf,
                                      const nghttp2_priority_spec *pri_spec);

/**
 * Unpacks the priority specification from payload |payload| of length
 * |payloadlen| to |pri_spec|.  The |flags| is used to determine what
 * kind of priority specification is in |payload|.  This function
 * assumes the |payload| contains whole priority specification.
 */
void nghttp2_frame_unpack_priority_spec(nghttp2_priority_spec *pri_spec,
                                        uint8_t flags,
                                        const uint8_t *payload,
                                        size_t payloadlen);

/*
 * Returns the offset from the HEADERS frame payload where the
 * compressed header block starts. The frame payload does not include
 * frame header.
 */
size_t nghttp2_frame_headers_payload_nv_offset(nghttp2_headers *frame);

/*
 * Packs HEADERS frame |frame| in wire format and store it in |bufs|.
 * This function expands |bufs| as necessary to store frame.
 *
 * The caller must make sure that nghttp2_bufs_reset(bufs) is called
 * before calling this function.
 *
 * frame->hd.length is assigned after length is determined during
 * packing process.  CONTINUATION frames are also serialized in this
 * function. This function does not handle padding.
 *
 * This function returns 0 if it succeeds, or returns one of the
 * following negative error codes:
 *
 * NGHTTP2_ERR_HEADER_COMP
 *     The deflate operation failed.
 * NGHTTP2_ERR_NOMEM
 *     Out of memory.
 */
int nghttp2_frame_pack_headers(nghttp2_bufs *bufs,
                               nghttp2_headers *frame,
                               nghttp2_hd_deflater *deflater);

/*
 * Unpacks HEADERS frame byte sequence into |frame|.  This function
 * only unapcks bytes that come before name/value header block and
 * after PAD_HIGH and PAD_LOW.
 *
 * This function always succeeds and returns 0.
 */
int nghttp2_frame_unpack_headers_payload(nghttp2_headers *frame,
                                         const uint8_t *payload,
                                         size_t payloadlen);

/*
 * Packs PRIORITY frame |frame| in wire format and store it in
 * |bufs|.
 *
 * The caller must make sure that nghttp2_bufs_reset(bufs) is called
 * before calling this function.
 *
 * This function always succeeds and returns 0.
 */
int nghttp2_frame_pack_priority(nghttp2_bufs *bufs,
                                nghttp2_priority *frame);

/*
 * Unpacks PRIORITY wire format into |frame|.
 */
void nghttp2_frame_unpack_priority_payload(nghttp2_priority *frame,
                                           const uint8_t *payload,
                                           size_t payloadlen);

/*
 * Packs RST_STREAM frame |frame| in wire frame format and store it in
 * |bufs|.
 *
 * The caller must make sure that nghttp2_bufs_reset(bufs) is called
 * before calling this function.
 *
 * This function always succeeds and returns 0.
 */
int nghttp2_frame_pack_rst_stream(nghttp2_bufs *bufs,
                                  nghttp2_rst_stream *frame);

/*
 * Unpacks RST_STREAM frame byte sequence into |frame|.
 */
void nghttp2_frame_unpack_rst_stream_payload(nghttp2_rst_stream *frame,
                                             const uint8_t *payload,
                                             size_t payloadlen);

/*
 * Packs SETTINGS frame |frame| in wire format and store it in
 * |bufs|.
 *
 * The caller must make sure that nghttp2_bufs_reset(bufs) is called
 * before calling this function.
 *
 * This function returns 0 if it succeeds, or returns one of the
 * following negative error codes:
 *
 * NGHTTP2_ERR_FRAME_SIZE_ERROR
 *     The length of the frame is too large.
 */
int nghttp2_frame_pack_settings(nghttp2_bufs *bufs, nghttp2_settings *frame);

/*
 * Packs the |iv|, which includes |niv| entries, in the |buf|,
 * assuming the |buf| has at least 8 * |niv| bytes.
 *
 * Returns the number of bytes written into the |buf|.
 */
size_t nghttp2_frame_pack_settings_payload(uint8_t *buf,
                                           const nghttp2_settings_entry *iv,
                                           size_t niv);

void nghttp2_frame_unpack_settings_entry(nghttp2_settings_entry *iv,
                                         const uint8_t *payload);

/*
 * Makes a copy of |iv| in frame->settings.iv. The |niv| is assigned
 * to frame->settings.niv.
 *
 * This function returns 0 if it succeeds or one of the following
 * negative error codes:
 *
 * NGHTTP2_ERR_NOMEM
 *     Out of memory.
 */
int nghttp2_frame_unpack_settings_payload(nghttp2_settings *frame,
                                          nghttp2_settings_entry *iv,
                                          size_t niv);

/*
 * Unpacks SETTINGS payload into |*iv_ptr|. The number of entries are
 * assigned to the |*niv_ptr|. This function allocates enough memory
 * to store the result in |*iv_ptr|. The caller is responsible to free
 * |*iv_ptr| after its use.
 *
 * This function returns 0 if it succeeds or one of the following
 * negative error codes:
 *
 * NGHTTP2_ERR_NOMEM
 *     Out of memory.
 */
int nghttp2_frame_unpack_settings_payload2(nghttp2_settings_entry **iv_ptr,
                                           size_t *niv_ptr,
                                           const uint8_t *payload,
                                           size_t payloadlen);

/*
 * Packs PUSH_PROMISE frame |frame| in wire format and store it in
 * |bufs|.  This function expands |bufs| as necessary to store
 * frame.
 *
 * The caller must make sure that nghttp2_bufs_reset(bufs) is called
 * before calling this function.
 *
 * frame->hd.length is assigned after length is determined during
 * packing process.  CONTINUATION frames are also serialized in this
 * function. This function does not handle padding.
 *
 * This function returns 0 if it succeeds, or returns one of the
 * following negative error codes:
 *
 * NGHTTP2_ERR_HEADER_COMP
 *     The deflate operation failed.
 * NGHTTP2_ERR_NOMEM
 *     Out of memory.
 */
int nghttp2_frame_pack_push_promise(nghttp2_bufs *bufs,
                                    nghttp2_push_promise *frame,
                                    nghttp2_hd_deflater *deflater);

/*
 * Unpacks PUSH_PROMISE frame byte sequence into |frame|.  This
 * function only unapcks bytes that come before name/value header
 * block and after PAD_HIGH and PAD_LOW.
 *
 * This function returns 0 if it succeeds or one of the following
 * negative error codes:
 *
 * NGHTTP2_ERR_PROTO
 *     TODO END_HEADERS flag is not set
 */
int nghttp2_frame_unpack_push_promise_payload(nghttp2_push_promise *frame,
                                              const uint8_t *payload,
                                              size_t payloadlen);

/*
 * Packs PING frame |frame| in wire format and store it in
 * |bufs|.
 *
 * The caller must make sure that nghttp2_bufs_reset(bufs) is called
 * before calling this function.
 *
 * This function always succeeds and returns 0.
 */
int nghttp2_frame_pack_ping(nghttp2_bufs *bufs, nghttp2_ping *frame);

/*
 * Unpacks PING wire format into |frame|.
 */
void nghttp2_frame_unpack_ping_payload(nghttp2_ping *frame,
                                       const uint8_t *payload,
                                       size_t payloadlen);

/*
 * Packs GOAWAY frame |frame| in wire format and store it in |bufs|.
 * This function expands |bufs| as necessary to store frame.
 *
 * The caller must make sure that nghttp2_bufs_reset(bufs) is called
 * before calling this function.
 *
 * This function returns 0 if it succeeds or one of the following
 * negative error codes:
 *
 * NGHTTP2_ERR_NOMEM
 *     Out of memory.
 * NGHTTP2_ERR_FRAME_SIZE_ERROR
 *     The length of the frame is too large.
 */
int nghttp2_frame_pack_goaway(nghttp2_bufs *bufs, nghttp2_goaway *frame);

/*
 * Unpacks GOAWAY wire format into |frame|.  The |payload| of length
 * |payloadlen| contains first 8 bytes of payload.  The
 * |var_gift_payload| of length |var_gift_payloadlen| contains
 * remaining payload and its buffer is gifted to the function and then
 * |frame|.  The |var_gift_payloadlen| must be freed by
 * nghttp2_frame_goaway_free().
 */
void nghttp2_frame_unpack_goaway_payload(nghttp2_goaway *frame,
                                         const uint8_t *payload,
                                         size_t payloadlen,
                                         uint8_t *var_gift_payload,
                                         size_t var_gift_payloadlen);

/*
 * Unpacks GOAWAY wire format into |frame|.  This function only exists
 * for unit test.  After allocating buffer for debug data, this
 * function internally calls nghttp2_frame_unpack_goaway_payload().
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * NGHTTP2_ERR_NOMEM
 *     Out of memory.
 */
int nghttp2_frame_unpack_goaway_payload2(nghttp2_goaway *frame,
                                         const uint8_t *payload,
                                         size_t payloadlen);

/*
 * Packs WINDOW_UPDATE frame |frame| in wire frame format and store it
 * in |bufs|.
 *
 * The caller must make sure that nghttp2_bufs_reset(bufs) is called
 * before calling this function.
 *
 * This function always succeeds and returns 0.
 */
int nghttp2_frame_pack_window_update(nghttp2_bufs *bufs,
                                     nghttp2_window_update *frame);

/*
 * Unpacks WINDOW_UPDATE frame byte sequence into |frame|.
 */
void nghttp2_frame_unpack_window_update_payload(nghttp2_window_update *frame,
                                                const uint8_t *payload,
                                                size_t payloadlen);

/*
 * Packs ALTSVC frame |frame| in wire format and store it in |bufs|.
 * This function expands |bufs| as necessary to store frame.
 *
 * The caller must make sure that nghttp2_bufs_reset(bufs) is called
 * before calling this function.
 *
 * This function returns 0 if it succeeds or one of the following
 * negative error codes:
 *
 * NGHTTP2_ERR_NOMEM
 *     Out of memory.
 * NGHTTP2_ERR_FRAME_SIZE_ERROR
 *     The length of the frame is too large.
 */
int nghttp2_frame_pack_altsvc(nghttp2_bufs *bufs, nghttp2_altsvc *frame);


/*
 * Unpacks ALTSVC frame byte sequence into |frame|.
 * The |payload| of length |payloadlen| contains first 8 bytes of
 * payload.  The |var_gift_payload| of length |var_gift_payloadlen|
 * contains remaining payload and its buffer is gifted to the function
 * and then |frame|.  The |var_gift_payloadlen| must be freed by
 * nghttp2_frame_altsvc_free().
 *
 * This function returns 0 if it succeeds or one of the following
 * negative error codes:
 *
 * NGHTTP2_ERR_FRAME_SIZE_ERROR
 *   The |var_gift_payload| does not contain required data.
 */
int nghttp2_frame_unpack_altsvc_payload(nghttp2_altsvc *frame,
                                        const uint8_t *payload,
                                        size_t payloadlen,
                                        uint8_t *var_gift_payload,
                                        size_t var_gift_payloadlen);

/*
 * Packs BLOCKED frame |frame| in wire format and store it in |bufs|.
 *
 * The caller must make sure that nghttp2_bufs_reset(bufs) is called
 * before calling this function.
 *
 * This function always returns 0.
 */
int nghttp2_frame_pack_blocked(nghttp2_bufs *bufs, nghttp2_blocked *frame);

/*
 * Initializes HEADERS frame |frame| with given values.  |frame| takes
 * ownership of |nva|, so caller must not free it. If |stream_id| is
 * not assigned yet, it must be -1.
 */
void nghttp2_frame_headers_init(nghttp2_headers *frame,
                                uint8_t flags, int32_t stream_id,
                                const nghttp2_priority_spec *pri_spec,
                                nghttp2_nv *nva, size_t nvlen);

void nghttp2_frame_headers_free(nghttp2_headers *frame);


void nghttp2_frame_priority_init(nghttp2_priority *frame, int32_t stream_id,
                                 const nghttp2_priority_spec *pri_spec);

void nghttp2_frame_priority_free(nghttp2_priority *frame);

void nghttp2_frame_rst_stream_init(nghttp2_rst_stream *frame,
                                   int32_t stream_id,
                                   nghttp2_error_code error_code);

void nghttp2_frame_rst_stream_free(nghttp2_rst_stream *frame);

/*
 * Initializes PUSH_PROMISE frame |frame| with given values.  |frame|
 * takes ownership of |nva|, so caller must not free it.
 */
void nghttp2_frame_push_promise_init(nghttp2_push_promise *frame,
                                     uint8_t flags, int32_t stream_id,
                                     int32_t promised_stream_id,
                                     nghttp2_nv *nva, size_t nvlen);

void nghttp2_frame_push_promise_free(nghttp2_push_promise *frame);

/*
 * Initializes SETTINGS frame |frame| with given values. |frame| takes
 * ownership of |iv|, so caller must not free it. The |flags| are
 * bitwise-OR of one or more of nghttp2_settings_flag.
 */
void nghttp2_frame_settings_init(nghttp2_settings *frame, uint8_t flags,
                                 nghttp2_settings_entry *iv, size_t niv);

void nghttp2_frame_settings_free(nghttp2_settings *frame);

/*
 * Initializes PING frame |frame| with given values. If the
 * |opqeue_data| is not NULL, it must point to 8 bytes memory region
 * of data. The data pointed by |opaque_data| is copied. It can be
 * NULL. In this case, 8 bytes NULL is used.
 */
void nghttp2_frame_ping_init(nghttp2_ping *frame, uint8_t flags,
                             const uint8_t *opque_data);

void nghttp2_frame_ping_free(nghttp2_ping *frame);

/*
 * Initializes GOAWAY frame |frame| with given values. On success,
 * this function takes ownership of |opaque_data|, so caller must not
 * free it. If the |opaque_data_len| is 0, opaque_data could be NULL.
 */
void nghttp2_frame_goaway_init(nghttp2_goaway *frame, int32_t last_stream_id,
                               nghttp2_error_code error_code,
                               uint8_t *opaque_data, size_t opaque_data_len);

void nghttp2_frame_goaway_free(nghttp2_goaway *frame);

void nghttp2_frame_window_update_init(nghttp2_window_update *frame,
                                      uint8_t flags,
                                      int32_t stream_id,
                                      int32_t window_size_increment);

void nghttp2_frame_window_update_free(nghttp2_window_update *frame);

/* protocol_id, host and origin must be allocated to the one chunk of
   memory region and protocol_id must point to it.  We only free
   protocol_id.  This means that |protocol_id| is not NULL even if
   |protocol_id_len| == 0 and |host_len| + |origin_len| > 0.  If
   |protocol_id_len|, |host_len| and |origin_len| are all zero,
   |protocol_id| can be NULL. */
void nghttp2_frame_altsvc_init(nghttp2_altsvc *frame, int32_t stream_id,
                               uint32_t max_age,
                               uint16_t port,
                               uint8_t *protocol_id,
                               size_t protocol_id_len,
                               uint8_t *host, size_t host_len,
                               uint8_t *origin, size_t origin_len);

void nghttp2_frame_altsvc_free(nghttp2_altsvc *frame);

void nghttp2_frame_blocked_init(nghttp2_blocked *frame, int32_t stream_id);

void nghttp2_frame_blocked_free(nghttp2_blocked *frame);

void nghttp2_frame_data_init(nghttp2_data *frame, nghttp2_private_data *pdata);

/*
 * Returns the number of padding bytes after payload. The total
 * padding length is given in the |padlen|. The returned value does
 * not include the PAD_HIGH and PAD_LOW.
 */
size_t nghttp2_frame_trail_padlen(nghttp2_frame *frame, size_t padlen);

void nghttp2_frame_private_data_init(nghttp2_private_data *frame,
                                     uint8_t flags,
                                     int32_t stream_id,
                                     const nghttp2_data_provider *data_prd);

void nghttp2_frame_private_data_free(nghttp2_private_data *frame);

/*
 * Makes copy of |iv| and return the copy. The |niv| is the number of
 * entries in |iv|. This function returns the pointer to the copy if
 * it succeeds, or NULL.
 */
nghttp2_settings_entry* nghttp2_frame_iv_copy(const nghttp2_settings_entry *iv,
                                              size_t niv);

/*
 * Sorts the |nva| in ascending order of name and value. If names are
 * equivalent, sort them by value.
 */
void nghttp2_nv_array_sort(nghttp2_nv *nva, size_t nvlen);

/*
 * Copies name/value pairs from |nva|, which contains |nvlen| pairs,
 * to |*nva_ptr|, which is dynamically allocated so that all items can
 * be stored.
 *
 * The |*nva_ptr| must be freed using nghttp2_nv_array_del().
 *
 * This function returns the number of name/value pairs in |*nva_ptr|,
 * or one of the following negative error codes:
 *
 * NGHTTP2_ERR_NOMEM
 *     Out of memory.
 */
ssize_t nghttp2_nv_array_copy(nghttp2_nv **nva_ptr,
                              const nghttp2_nv *nva, size_t nvlen);

/*
 * Returns nonzero if the name/value pair |a| equals to |b|. The name
 * is compared in case-sensitive, because we ensure that this function
 * is called after the name is lower-cased.
 */
int nghttp2_nv_equal(const nghttp2_nv *a, const nghttp2_nv *b);

/*
 * Frees |nva|.
 */
void nghttp2_nv_array_del(nghttp2_nv *nva);

/*
 * Checks that the |iv|, which includes |niv| entries, does not have
 * invalid values.
 *
 * This function returns nonzero if it succeeds, or 0.
 */
int nghttp2_iv_check(const nghttp2_settings_entry *iv, size_t niv);

/*
 * Sets PAD_HIGH and PAD_LOW fields, flags and adjust frame header
 * position of each buffers in |bufs|.  The padding is given in the
 * |padlen|. The |hd| is the frame header for the serialized data.
 * The |type| is used as a frame type when padding requires additional
 * buffers.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * NGHTTP2_ERR_NOMEM
 *     Out of memory.
 * NGHTTP2_ERR_FRAME_SIZE_ERROR
 *     The length of the resulting frame is too large.
 */
int nghttp2_frame_add_pad(nghttp2_bufs *bufs, nghttp2_frame_hd *hd,
                          size_t padlen, nghttp2_frame_type type);

#endif /* NGHTTP2_FRAME_H */
