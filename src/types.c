// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <mpv/client.h>
#include "mpv_talloc.h"
#include "misc/ctype.h"
#include "types.h"

static void update_track_list(mp_state *state, mpv_node *node) {
    if (state->track_list != NULL) talloc_free(state->track_list);
    state->track_list = talloc_zero(state, mp_track_list);
    mp_track_list *list = state->track_list;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node track = node->u.list->values[i];

        mp_track_item item = {0};

        for (int j = 0; j < track.u.list->num; j++) {
            char *key = track.u.list->keys[j];
            mpv_node value = track.u.list->values[j];
            if (strcmp(key, "id") == 0) {
                item.id = value.u.int64;
            } else if (strcmp(key, "title") == 0) {
                item.title = talloc_strdup(list, value.u.string);
            } else if (strcmp(key, "lang") == 0) {
                item.lang = talloc_strdup(list, value.u.string);
            } else if (strcmp(key, "type") == 0) {
                item.type = talloc_strdup(list, value.u.string);
            } else if (strcmp(key, "selected") == 0) {
                item.selected = value.u.flag;
            }
        }

        // set default title if not set
        if (item.title == NULL)
            item.title = talloc_asprintf(list, "%s %d", item.type, item.id);

        // convert lang to uppercase
        if (item.lang != NULL) {
            for (int x = 0; item.lang[x]; x++)
                item.lang[x] = mp_toupper(item.lang[x]);
        }

        MP_TARRAY_APPEND(list, list->entries, list->num_entries, item);
    }
}

static void update_chapter_list(mp_state *state, mpv_node *node) {
    if (state->chapter_list != NULL) talloc_free(state->chapter_list);
    state->chapter_list = talloc_zero(state, mp_chapter_list);
    mp_chapter_list *list = state->chapter_list;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node chapter = node->u.list->values[i];

        mp_chapter_item item = {0};

        for (int j = 0; j < chapter.u.list->num; j++) {
            char *key = chapter.u.list->keys[j];
            mpv_node value = chapter.u.list->values[j];
            if (strcmp(key, "title") == 0) {
                item.title = talloc_strdup(list, value.u.string);
            } else if (strcmp(key, "time") == 0) {
                item.time = value.u.double_;
            }
        }

        // set default title if not set
        if (item.title == NULL)
            item.title = talloc_asprintf(list, "chapter %d", i + 1);

        MP_TARRAY_APPEND(list, list->entries, list->num_entries, item);
    }
}

static void update_edition_list(mp_state *state, mpv_node *node) {
    if (state->edition_list != NULL) talloc_free(state->edition_list);
    state->edition_list = talloc_zero(state, mp_edition_list);
    mp_edition_list *list = state->edition_list;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node edition = node->u.list->values[i];

        mp_edition_item item = {0};

        for (int j = 0; j < edition.u.list->num; j++) {
            char *key = edition.u.list->keys[j];
            mpv_node value = edition.u.list->values[j];
            if (strcmp(key, "title") == 0) {
                item.title = talloc_strdup(list, value.u.string);
            } else if (strcmp(key, "id") == 0) {
                item.id = value.u.int64;
            }
        }

        // set default title if not set
        if (item.title == NULL)
            item.title = talloc_asprintf(list, "edition %d", item.id);

        MP_TARRAY_APPEND(list, list->entries, list->num_entries, item);
    }
}

static void update_audio_device_list(mp_state *state, mpv_node *node) {
    if (state->audio_device_list != NULL) talloc_free(state->audio_device_list);
    state->audio_device_list = talloc_zero(state, mp_audio_device_list);
    mp_audio_device_list *list = state->audio_device_list;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node device = node->u.list->values[i];

        mp_audio_device item = {0};

        for (int j = 0; j < device.u.list->num; j++) {
            char *key = device.u.list->keys[j];
            mpv_node value = device.u.list->values[j];
            if (strcmp(key, "name") == 0) {
                item.name = talloc_strdup(list, value.u.string);
            } else if (strcmp(key, "description") == 0) {
                item.desc = talloc_strdup(list, value.u.string);
            }
        }

        MP_TARRAY_APPEND(list, list->entries, list->num_entries, item);
    }
}

void mp_state_init(mp_state *state, mpv_handle *mpv) {
    mpv_observe_property(mpv, 0, "vid", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "aid", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "sid", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "secondary-sid", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "chapter", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "current-edition", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "audio-device", MPV_FORMAT_STRING);
    mpv_observe_property(mpv, 0, "track-list", MPV_FORMAT_NODE);
    mpv_observe_property(mpv, 0, "chapter-list", MPV_FORMAT_NODE);
    mpv_observe_property(mpv, 0, "edition-list", MPV_FORMAT_NODE);
    mpv_observe_property(mpv, 0, "audio-device-list", MPV_FORMAT_NODE);
}

void mp_state_update(mp_state *state, mpv_event *event) {
    mpv_event_property *prop = event->data;
    switch (prop->format) {
        case MPV_FORMAT_NONE:
            if (strcmp(prop->name, "vid") == 0) {
                state->vid = -1;
            } else if (strcmp(prop->name, "aid") == 0) {
                state->aid = -1;
            } else if (strcmp(prop->name, "sid") == 0) {
                state->sid = -1;
            } else if (strcmp(prop->name, "secondary-sid") == 0) {
                state->sid2 = -1;
            }
            break;
        case MPV_FORMAT_INT64:
            if (strcmp(prop->name, "vid") == 0) {
                state->vid = *(int64_t *)prop->data;
            } else if (strcmp(prop->name, "aid") == 0) {
                state->aid = *(int64_t *)prop->data;
            } else if (strcmp(prop->name, "sid") == 0) {
                state->sid = *(int64_t *)prop->data;
            } else if (strcmp(prop->name, "secondary-sid") == 0) {
                state->sid2 = *(int64_t *)prop->data;
            } else if (strcmp(prop->name, "chapter") == 0) {
                state->chapter = *(int64_t *)prop->data;
            } else if (strcmp(prop->name, "current-edition") == 0) {
                state->edition = *(int64_t *)prop->data;
            }
            break;
        case MPV_FORMAT_STRING:
            if (strcmp(prop->name, "audio-device") == 0) {
                if (state->audio_device != NULL)
                    talloc_free(state->audio_device);
                char *val = *(char **)prop->data;
                state->audio_device = talloc_strdup(state, val);
            }
            break;
        case MPV_FORMAT_NODE:
            if (strcmp(prop->name, "track-list") == 0) {
                update_track_list(state, prop->data);
            } else if (strcmp(prop->name, "chapter-list") == 0) {
                update_chapter_list(state, prop->data);
            } else if (strcmp(prop->name, "edition-list") == 0) {
                update_edition_list(state, prop->data);
            } else if (strcmp(prop->name, "audio-device-list") == 0) {
                update_audio_device_list(state, prop->data);
            }
            break;
    }
}