// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_H
#define MPV_PLUGIN_H

#include <windows.h>
#include <mpv/client.h>

#include "osdep/threads.h"
#include "misc/dispatch.h"

typedef struct plugin_config {
    bool uosc;  // use uosc menu syntax
} plugin_config;

typedef struct plugin_ctx {
    struct plugin_config *conf;  // plugin config

    struct mp_dispatch_queue *dispatch;  // dispatch queue
    mp_thread thread;                    // dispatch thread
    bool terminate;                      // terminate thread

    mpv_handle *mpv;   // mpv client handle
    HWND hwnd;         // window handle
    HMENU hmenu;       // menu handle
    WNDPROC wnd_proc;  // previous window procedure
} plugin_ctx;

typedef struct track_item {
    int64_t id;     // the ID as it's used for -sid/--aid/--vid
    char *type;     // string describing the media type
    char *title;    // track title as it is stored in the file
    char *lang;     // track language as identified by the file
    bool selected;  // if the track is currently decoded
} track_item;

typedef struct mp_track_list {
    struct track_item *entries;
    int num_entries;
} mp_track_list;

typedef struct chapter_item {
    char *title;  // chapter title as stored in the file
    double time;  // chapter start time in seconds as float
} chapter_item;

typedef struct mp_chapter_list {
    struct chapter_item *entries;
    int num_entries;
} mp_chapter_list;

typedef struct edition_item {
    char *title;  // edition title as stored in the file
    int64_t id;   // edition ID as integer
} edition_item;

typedef struct mp_edition_list {
    struct edition_item *entries;
    int num_entries;
} mp_edition_list;

typedef struct audio_device {
    char *name;  // device name
    char *desc;  // device description
} audio_device;

typedef struct mp_audio_device_list {
    struct audio_device *entries;
    int num_entries;
} mp_audio_device_list;

wchar_t *mp_from_utf8(void *talloc_ctx, const char *s);
char *mp_get_prop_string(void *talloc_ctx, const char *prop);
int64_t mp_get_prop_int(const char *prop);
mp_track_list *mp_get_track_list(void *talloc_ctx, const char *type);
mp_chapter_list *mp_get_chapter_list(void *talloc_ctx);
mp_edition_list *mp_get_edition_list(void *talloc_ctx);
mp_audio_device_list *mp_get_audio_device_list(void *talloc_ctx);
char *mp_expand_path(void *talloc_ctx, char *path);
char *mp_read_file(void *talloc_ctx, char *path);
void mp_command_async(const char *args);

#endif