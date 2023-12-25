// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_TYPES_H
#define MPV_PLUGIN_TYPES_H

#include <mpv/client.h>

typedef struct {
    int64_t id;     // the ID as it's used for -sid/--aid/--vid
    char *type;     // string describing the media type
    char *title;    // track title as it is stored in the file
    char *lang;     // track language as identified by the file
    bool selected;  // if the track is currently decoded
} mp_track_item;

typedef struct {
    mp_track_item *entries;
    int num_entries;
} mp_track_list;

typedef struct {
    char *title;  // chapter title as stored in the file
    double time;  // chapter start time in seconds as float
} mp_chapter_item;

typedef struct {
    mp_chapter_item *entries;
    int num_entries;
} mp_chapter_list;

typedef struct {
    char *title;  // edition title as stored in the file
    int64_t id;   // edition ID as integer
} mp_edition_item;

typedef struct {
    mp_edition_item *entries;
    int num_entries;
} mp_edition_list;

typedef struct {
    char *name;  // device name
    char *desc;  // device description
} mp_audio_device;

typedef struct {
    mp_audio_device *entries;
    int num_entries;
} mp_audio_device_list;

typedef struct {
    int64_t vid;      // video ID
    int64_t aid;      // audio ID
    int64_t sid;      // subtitle ID
    int64_t sid2;     // secondary subtitle ID
    int64_t chapter;  // chapter ID
    int64_t edition;  // edition ID

    char *audio_device;  // audio device name

    mp_track_list *track_list;                // track list
    mp_chapter_list *chapter_list;            // chapter list
    mp_edition_list *edition_list;            // edition list
    mp_audio_device_list *audio_device_list;  // audio device list
} mp_state;

void update_track_list(mp_state *state, mpv_node *node);
void update_chapter_list(mp_state *state, mpv_node *node);
void update_edition_list(mp_state *state, mpv_node *node);
void update_audio_device_list(mp_state *state, mpv_node *node);

#endif