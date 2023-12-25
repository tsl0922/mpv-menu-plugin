// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <mpv/client.h>

#include "misc/bstr.h"
#include "misc/ctype.h"
#include "menu.h"
#include "plugin.h"

struct plugin_ctx *ctx = NULL;

#define WM_SHOWMENU (WM_USER + 1)

// forward declarations
static void update_track_list(mp_state *state, mpv_node *node);
static void update_chapter_list(mp_state *state, mpv_node *node);
static void update_edition_list(mp_state *state, mpv_node *node);
static void update_audio_device_list(mp_state *state, mpv_node *node);

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    POINT pt;

    switch (uMsg) {
        case WM_SHOWMENU:
            if (GetCursorPos(&pt)) show_menu(ctx, &pt);
            break;
        case WM_COMMAND:
            handle_menu(ctx, LOWORD(wParam));
            break;
        default:
            break;
    }

    return CallWindowProcW(ctx->wnd_proc, hWnd, uMsg, wParam, lParam);
}

static void plugin_read_conf(plugin_config *conf, const char *name) {
    void *tmp = talloc_new(NULL);
    char *path = talloc_asprintf(tmp, "~~/script-opts/%s.conf", name);
    bstr data = bstr0(mp_read_file(tmp, path));

    while (data.len) {
        bstr line = bstr_strip_linebreaks(bstr_getline(data, &data));
        line = bstr_lstrip(line);
        if (line.len == 0 || bstr_startswith0(line, "#")) continue;

        bstr name, value;
        if (!bstr_split_tok(line, "=", &name, &value)) continue;
        name = bstr_strip(name);
        value = bstr_strip(value);
        if (name.len == 0 || value.len == 0) continue;

        if (bstr_equals0(name, "uosc")) conf->uosc = bstr_equals0(value, "yes");
    }

    talloc_free(tmp);
}

static void plugin_register(int64_t wid) {
    ctx->hmenu = load_menu(ctx);
    ctx->hwnd = (HWND)wid;
    ctx->wnd_proc =
        (WNDPROC)SetWindowLongPtrW(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
}

static void handle_property_change(mpv_event *event) {
    mp_state *state = ctx->state;
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
            if (strcmp(prop->name, "window-id") == 0) {
                int64_t wid = *(int64_t *)prop->data;
                if (wid > 0) plugin_register(wid);
            } else if (strcmp(prop->name, "vid") == 0) {
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

static void handle_client_message(mpv_event *event) {
    mpv_event_client_message *msg = event->data;
    if (msg->num_args < 1) return;

    const char *cmd = msg->args[0];
    if (strcmp(cmd, "show") == 0) PostMessageW(ctx->hwnd, WM_SHOWMENU, 0, 0);
}

static plugin_ctx *create_plugin_ctx() {
    plugin_ctx *ctx = talloc_ptrtype(NULL, ctx);
    memset(ctx, 0, sizeof(*ctx));

    ctx->conf = talloc_ptrtype(ctx, ctx->conf);
    memset(ctx->conf, 0, sizeof(*ctx->conf));
    ctx->state = talloc_ptrtype(ctx, ctx->state);
    memset(ctx->state, 0, sizeof(*ctx->state));

    return ctx;
}

static void destroy_plugin_ctx() {
    if (ctx->hmenu) DestroyMenu(ctx->hmenu);
    if (ctx->hwnd && ctx->wnd_proc)
        SetWindowLongPtrW(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)ctx->wnd_proc);
    talloc_free(ctx);
}

MPV_EXPORT int mpv_open_cplugin(mpv_handle *handle) {
    ctx = create_plugin_ctx(NULL);
    ctx->mpv = handle;
    plugin_read_conf(ctx->conf, mpv_client_name(handle));

    ctx->dispatch = mp_dispatch_create(ctx);

    mpv_observe_property(handle, 0, "window-id", MPV_FORMAT_INT64);
    mpv_observe_property(handle, 0, "vid", MPV_FORMAT_INT64);
    mpv_observe_property(handle, 0, "aid", MPV_FORMAT_INT64);
    mpv_observe_property(handle, 0, "sid", MPV_FORMAT_INT64);
    mpv_observe_property(handle, 0, "secondary-sid", MPV_FORMAT_INT64);
    mpv_observe_property(handle, 0, "chapter", MPV_FORMAT_INT64);
    mpv_observe_property(handle, 0, "current-edition", MPV_FORMAT_INT64);
    mpv_observe_property(handle, 0, "audio-device", MPV_FORMAT_STRING);
    mpv_observe_property(handle, 0, "track-list", MPV_FORMAT_NODE);
    mpv_observe_property(handle, 0, "chapter-list", MPV_FORMAT_NODE);
    mpv_observe_property(handle, 0, "edition-list", MPV_FORMAT_NODE);
    mpv_observe_property(handle, 0, "audio-device-list", MPV_FORMAT_NODE);

    while (handle) {
        mpv_event *event = mpv_wait_event(handle, -1);
        if (event->event_id == MPV_EVENT_SHUTDOWN) break;

        mp_dispatch_queue_process(ctx->dispatch, 0);

        switch (event->event_id) {
            case MPV_EVENT_PROPERTY_CHANGE:
                handle_property_change(event);
                break;
            case MPV_EVENT_CLIENT_MESSAGE:
                handle_client_message(event);
                break;
            case MPV_EVENT_SHUTDOWN:
                destroy_plugin_ctx();
                break;
            default:
                break;
        }
    }

    return 0;
}

wchar_t *mp_from_utf8(void *talloc_ctx, const char *s) {
    int count = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (count <= 0) abort();
    wchar_t *ret = talloc_array(talloc_ctx, wchar_t, count);
    MultiByteToWideChar(CP_UTF8, 0, s, -1, ret, count);
    return ret;
}

char *mp_get_prop_string(void *talloc_ctx, const char *prop) {
    char *val = mpv_get_property_string(ctx->mpv, prop);
    char *ret = talloc_strdup(talloc_ctx, val);
    if (val != NULL) mpv_free(val);
    return ret;
}

static void update_track_list(mp_state *state, mpv_node *node) {
    if (state->track_list != NULL) talloc_free(state->track_list);
    state->track_list = talloc_ptrtype(state, state->track_list);
    memset(state->track_list, 0, sizeof(*state->track_list));
    mp_track_list *list = state->track_list;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node track = node->u.list->values[i];

        struct track_item item;
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

static void update_chapter_list(mp_state *state, mpv_node *node) {
    if (state->chapter_list != NULL) talloc_free(state->chapter_list);
    state->chapter_list = talloc_ptrtype(state, state->chapter_list);
    memset(state->chapter_list, 0, sizeof(*state->chapter_list));
    mp_chapter_list *list = state->chapter_list;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node chapter = node->u.list->values[i];

        struct chapter_item item;
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

static void update_edition_list(mp_state *state, mpv_node *node) {
    if (state->edition_list != NULL) talloc_free(state->edition_list);
    state->edition_list = talloc_ptrtype(state, state->edition_list);
    memset(state->edition_list, 0, sizeof(*state->edition_list));
    mp_edition_list *list = state->edition_list;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node edition = node->u.list->values[i];

        struct edition_item item;
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

static void update_audio_device_list(mp_state *state, mpv_node *node) {
    if (state->audio_device_list != NULL) talloc_free(state->audio_device_list);
    state->audio_device_list = talloc_ptrtype(state, state->audio_device_list);
    memset(state->audio_device_list, 0, sizeof(*state->audio_device_list));
    mp_audio_device_list *list = state->audio_device_list;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node device = node->u.list->values[i];

        struct audio_device item;
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

char *mp_expand_path(void *talloc_ctx, char *path) {
    mpv_node node;
    const char *args[] = {"expand-path", path, NULL};
    if (mpv_command_ret(ctx->mpv, args, &node) >= 0) {
        path = talloc_strdup(talloc_ctx, node.u.string);
        mpv_free_node_contents(&node);
    }
    return path;
}

static void async_cmd_fn(void *data) {
    mpv_command_string(ctx->mpv, (const char *)data);
}

void mp_command_async(const char *args) {
    mp_dispatch_enqueue(ctx->dispatch, async_cmd_fn, (void *)args);
    mpv_wakeup(ctx->mpv);
}

char *mp_read_file(void *talloc_ctx, char *path) {
    if (bstr_startswith0(bstr0(path), "memory://"))
        return talloc_strdup(talloc_ctx, path + strlen("memory://"));

    void *tmp = talloc_new(NULL);
    char *path_m = mp_expand_path(tmp, path);
    wchar_t *path_w = mp_from_utf8(tmp, path_m);
    HANDLE hFile = CreateFileW(path_w, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    talloc_free(tmp);
    if (hFile == INVALID_HANDLE_VALUE) return NULL;

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    char *ret = talloc_array(talloc_ctx, char, dwFileSize + 1);
    DWORD dwRead;

    ReadFile(hFile, ret, dwFileSize, &dwRead, NULL);
    ret[dwFileSize] = '\0';
    CloseHandle(hFile);

    return ret;
}
