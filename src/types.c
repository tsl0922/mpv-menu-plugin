// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <mpv/client.h>
#include "mpv_talloc.h"
#include "misc/ctype.h"
#include "types.h"

void update_track_list(mp_state *state, mpv_node *node) {
    if (state->track_list != NULL) talloc_free(state->track_list);
    state->track_list = talloc_ptrtype(state, state->track_list);
    memset(state->track_list, 0, sizeof(*state->track_list));
    mp_track_list *list = state->track_list;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node track = node->u.list->values[i];

        mp_track_item item;
        memset(&item, 0, sizeof(item));

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

void update_chapter_list(mp_state *state, mpv_node *node) {
    if (state->chapter_list != NULL) talloc_free(state->chapter_list);
    state->chapter_list = talloc_ptrtype(state, state->chapter_list);
    memset(state->chapter_list, 0, sizeof(*state->chapter_list));
    mp_chapter_list *list = state->chapter_list;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node chapter = node->u.list->values[i];

        mp_chapter_item item;
        memset(&item, 0, sizeof(item));

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

void update_edition_list(mp_state *state, mpv_node *node) {
    if (state->edition_list != NULL) talloc_free(state->edition_list);
    state->edition_list = talloc_ptrtype(state, state->edition_list);
    memset(state->edition_list, 0, sizeof(*state->edition_list));
    mp_edition_list *list = state->edition_list;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node edition = node->u.list->values[i];

        mp_edition_item item;
        memset(&item, 0, sizeof(item));

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

void update_audio_device_list(mp_state *state, mpv_node *node) {
    if (state->audio_device_list != NULL) talloc_free(state->audio_device_list);
    state->audio_device_list = talloc_ptrtype(state, state->audio_device_list);
    memset(state->audio_device_list, 0, sizeof(*state->audio_device_list));
    mp_audio_device_list *list = state->audio_device_list;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node device = node->u.list->values[i];

        mp_audio_device item;
        memset(&item, 0, sizeof(item));

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