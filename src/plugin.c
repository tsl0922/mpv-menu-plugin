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

// global plugin context
plugin_ctx *ctx = NULL;

#define WM_SHOWMENU (WM_USER + 1)

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
    mpv_event_property *prop = event->data;
    if (prop->format == MPV_FORMAT_INT64 &&
        strcmp(prop->name, "window-id") == 0) {
        int64_t wid = *(int64_t *)prop->data;
        if (wid > 0) plugin_register(wid);
    } else {
        mp_state_update(ctx->state, event);
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
    ctx->dispatch = mp_dispatch_create(ctx);

    plugin_read_conf(ctx->conf, mpv_client_name(handle));
    mpv_observe_property(handle, 0, "window-id", MPV_FORMAT_INT64);
    mp_state_init(ctx->state, handle);

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
