/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2014
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * xrdp / xserver info / caps
 */

#include "xrdp_constants.h"

#if !defined(XRDP_CLIENT_INFO_H)
#define XRDP_CLIENT_INFO_H

#define XRDP_MAXIMUM_MONITORS 16

/*
 * 2.2.1.3.6.1 Monitor Definition (TS_MONITOR_DEF)
 * 2.2.1.3.9.1 Monitor Attributes (TS_MONITOR_ATTRIBUTES)
 */
struct monitor_info
{
    int left;
    int top;
    int width;
    int height;
    int right;
    int bottom;
    int physical_width;
    int physical_height;
    int orientation;
    int desktop_scale_factor;
    int device_scale_factor;
    int is_primary;
    int flags;
};

enum display_resize_state {
    WMRZ_QUEUED = 0,
    WMRZ_ENCODER_DELETE,
    WMRZ_EGFX_DELETE_SURFACE,
    WMRZ_EGFX_CONN_CLOSE,
    WMRZ_EGFX_CONN_CLOSING,
    WMRZ_EGFX_CONN_CLOSED,
    WRMZ_EGFX_DELETE,
    WMRZ_SERVER_MONITOR_RESIZE,
    WMRZ_SERVER_VERSION_MESSAGE_START,
    WMRZ_SERVER_MONITOR_MESSAGE_PROCESSING,
    WMRZ_SERVER_MONITOR_MESSAGE_PROCESSED,
    WMRZ_XRDP_CORE_RESIZE,
    WMRZ_EGFX_INITIALIZE,
    WMRZ_EGFX_INITALIZING,
    WMRZ_EGFX_INITIALIZED,
    WMRZ_SERVER_INVALIDATE,
    WMRZ_COMPLETE,
    WMRZ_ERROR
};

#define XRDP_DISPLAY_RESIZE_STATE_TO_STR(status) \
    ((status) == WMRZ_QUEUED ? "QUEUED" : \
     (status) == WMRZ_ENCODER_DELETE ? "ENCODER_DELETE" : \
     (status) == WMRZ_EGFX_DELETE_SURFACE ? "EGFX_DELETE_SURFACE" : \
     (status) == WMRZ_EGFX_CONN_CLOSE ? "EGFX_CONN_CLOSE" : \
     (status) == WMRZ_EGFX_CONN_CLOSING ? "EGFX_CONN_CLOSING" : \
     (status) == WMRZ_EGFX_CONN_CLOSED ? "EGFX_CONN_CLOSED" : \
     (status) == WRMZ_EGFX_DELETE ? "EGFX_DELETE" : \
     (status) == WMRZ_SERVER_MONITOR_RESIZE ? "SERVER_MONITOR_RESIZE" : \
     (status) == WMRZ_SERVER_VERSION_MESSAGE_START ? "SERVER_VERSION_MESSAGE_START" : \
     (status) == WMRZ_SERVER_MONITOR_MESSAGE_PROCESSING ? "SERVER_MONITOR_MESSAGE_PROCESSING" : \
     (status) == WMRZ_SERVER_MONITOR_MESSAGE_PROCESSED ? "SERVER_MONITOR_MESSAGE_PROCESSED" : \
     (status) == WMRZ_XRDP_CORE_RESIZE ? "XRDP_CORE_RESIZE" : \
     (status) == WMRZ_EGFX_INITIALIZE ? "EGFX_INITIALIZE" : \
     (status) == WMRZ_EGFX_INITALIZING ? "EGFX_INITALIZING" : \
     (status) == WMRZ_EGFX_INITIALIZED ? "EGFX_INITIALIZED" : \
     (status) == WMRZ_SERVER_INVALIDATE ? "SERVER_INVALIDATE" : \
     (status) == WMRZ_COMPLETE ? "COMPLETE" : \
     (status) == WMRZ_ERROR ? "ERROR" : \
     "unknown" \
    )

struct display_size_description
{
    int monitorCount; /* number of monitors detected (max = 16) */
    struct monitor_info minfo[XRDP_MAXIMUM_MONITORS]; /* client monitor data */
    struct monitor_info minfo_wm[XRDP_MAXIMUM_MONITORS]; /* client monitor data, non-negative values */
    int session_width;
    int session_height;
    enum display_resize_state state;
    int last_state_update_timestamp;
    int start_time;
};

/**
 * Information about the xrdp client
 *
 * @note This structure is shared with xorgxrdp. If you change anything
 *       above the 'private to xrdp below this line' comment, you MUST
 *       bump the CLIENT_INFO_CURRENT_VERSION number so that the mismatch
 *       can be detected.
 */
struct xrdp_client_info
{
    int size; /* bytes for this structure */
    int version; /* Should be CLIENT_INFO_CURRENT_VERSION */
    int bpp;
    int width;
    int height;
    /* bitmap cache info */
    int cache1_entries;
    int cache1_size;
    int cache2_entries;
    int cache2_size;
    int cache3_entries;
    int cache3_size;
    int bitmap_cache_persist_enable; /* 0 or 2 */
    int bitmap_cache_version; /* ored 1 = original version, 2 = v2, 4 = v3 */
    /* pointer info */
    int pointer_cache_entries;
    /* other */
    int use_bitmap_comp;
    int use_bitmap_cache;
    int op1; /* use smaller bitmap header, non cache */
    int op2; /* use smaller bitmap header in bitmap cache */
    int desktop_cache;
    int use_compact_packets; /* rdp5 smaller packets */
    char hostname[32];
    int build;
    int keylayout;
    char username[INFO_CLIENT_MAX_CB_LEN];
    char password[INFO_CLIENT_MAX_CB_LEN];
    char domain[INFO_CLIENT_MAX_CB_LEN];
    char program[INFO_CLIENT_MAX_CB_LEN];
    char directory[INFO_CLIENT_MAX_CB_LEN];
    int rdp_compression;
    int rdp_autologin;
    int crypt_level; /* 1, 2, 3 = low, medium, high */
    int channels_allowed; /* 0 = no channels 1 = channels */
    int sound_code; /* 1 = leave sound at server */
    int is_mce;
    int rdp5_performanceflags;
    int brush_cache_code; /* 0 = no cache 1 = 8x8 standard cache
                           2 = arbitrary dimensions */
    char client_ip[256];
    int max_bpp;
    int jpeg; /* non standard bitmap cache v2 cap */
    int offscreen_support_level;
    int offscreen_cache_size;
    int offscreen_cache_entries;
    int rfx;

    /* CAPSETTYPE_RAIL */
    int rail_support_level;
    /* CAPSETTYPE_WINDOW */
    int wnd_support_level;
    int wnd_num_icon_caches;
    int wnd_num_icon_cache_entries;
    /* codecs */
    int rfx_codec_id;
    int rfx_prop_len;
    char rfx_prop[64];
    int ns_codec_id;
    int ns_prop_len;
    char ns_prop[64];
    int jpeg_codec_id;
    int jpeg_prop_len;
    char jpeg_prop[64];
    int v3_codec_id;
    int rfx_min_pixel;
    char orders[32];
    int order_flags_ex;
    int use_bulk_comp;
    int pointer_flags; /* 0 color, 1 new, 2 no new */
    int use_fast_path;
    int require_credentials; /* when true, credentials *must* be passed on cmd line */
    char client_addr[256];
    char client_port[256];

    int security_layer; /* 0 = rdp, 1 = tls , 2 = hybrid */
    int multimon; /* 0 = deny , 1 = allow */
    int monitorCount; /* number of monitors detected (max = 16) */
    struct monitor_info minfo[XRDP_MAXIMUM_MONITORS]; /* client monitor data */
    struct monitor_info minfo_wm[XRDP_MAXIMUM_MONITORS]; /* client monitor data, non-negative values */

    int keyboard_type;
    int keyboard_subtype;

    int png_codec_id;
    int png_prop_len;
    char png_prop[64];
    int vendor_flags[4];
    int mcs_connection_type;
    int mcs_early_capability_flags;

    int max_fastpath_frag_bytes;
    int capture_code;
    int capture_format;

    char certificate[1024];
    char key_file[1024];

    /* X11 keyboard layout - inferred from keyboard type/subtype */
    char model[16];
    char layout[16];
    char variant[16];
    char options[256];

    /* ==================================================================== */
    /* Private to xrdp below this line */
    /* ==================================================================== */

    /* codec */
    int h264_codec_id;
    int h264_prop_len;
    char h264_prop[64];

    int use_frame_acks;
    int max_unacknowledged_frame_count;

    long ssl_protocols;
    char *tls_ciphers;

    int client_os_major;
    int client_os_minor;

    int no_orders_supported;
    int use_cache_glyph_v2;
    int rail_enable;
    int suppress_output;

    int enable_token_login;
    char domain_user_separator[16];
    int gfx;
};

/* yyyymmdd of last incompatible change to xrdp_client_info */
#define CLIENT_INFO_CURRENT_VERSION 20210225

#endif
