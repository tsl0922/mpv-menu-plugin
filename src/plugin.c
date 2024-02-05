// Copyright (c) 2023-2024 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <shlwapi.h>
#include <mpv/client.h>
#include "mpv_talloc.h"
#include "dialog.h"
#include "menu.h"
#include "plugin.h"

// global plugin context
plugin_ctx *ctx = NULL;

// custom window messages
#define WM_SHOWMENU (WM_USER + 1)

// handle window messages
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

// get clipboard text, always return utf8 string
static char *get_clipboard(void *talloc_ctx) {
    if (!OpenClipboard(ctx->hwnd)) return NULL;

    // try to get unicode text first
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    char *ret = NULL;
    if (hData != NULL) {
        wchar_t *data = (wchar_t *)GlobalLock(hData);
        if (data != NULL) {
            ret = mp_to_utf8(talloc_ctx, data);
            GlobalUnlock(hData);
        }
        goto done;
    }

    // try to get file drop list
    hData = GetClipboardData(CF_HDROP);
    if (hData != NULL) {
        HDROP hDrop = (HDROP)GlobalLock(hData);
        if (hDrop != NULL) {
            int count = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
            if (count > 0) {
                void *tmp = talloc_new(NULL);
                ret = talloc_strdup(talloc_ctx, "");
                for (int i = 0; i < count; i++) {
                    int len = DragQueryFileW(hDrop, i, NULL, 0);
                    wchar_t *path_w = talloc_array(tmp, wchar_t, len + 1);
                    if (DragQueryFileW(hDrop, i, path_w, len + 1)) {
                        char *path = mp_to_utf8(tmp, path_w);
                        ret = talloc_asprintf_append(ret, "%s\n", path);
                    }
                }
                talloc_free(tmp);
            }
            GlobalUnlock(hData);
        }
    }

done:
    CloseClipboard();

    return ret;
}

// set clipboard text, always convert to wide string
static void set_clipboard(const char *text) {
    if (!OpenClipboard(ctx->hwnd)) return;
    EmptyClipboard();

    int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(wchar_t));
    if (hData != NULL) {
        wchar_t *data = (wchar_t *)GlobalLock(hData);
        if (data != NULL) {
            MultiByteToWideChar(CP_UTF8, 0, text, -1, data, len);
            GlobalUnlock(hData);
            SetClipboardData(CF_UNICODETEXT, hData);
        }
    }
    CloseClipboard();
}

// create HMENU and register window procedure
static void plugin_register(int64_t wid) {
    ctx->hwnd = (HWND)wid;
    ctx->wnd_proc =
        (WNDPROC)SetWindowLongPtrW(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
}

// handle property change event
static void handle_property_change(mpv_event *event) {
    mpv_event_property *prop = event->data;
    switch (prop->format) {
        case MPV_FORMAT_INT64:
            if (strcmp(prop->name, "window-id") == 0) {
                int64_t wid = *(int64_t *)prop->data;
                if (wid > 0) plugin_register(wid);
            }
            break;
        case MPV_FORMAT_NODE:
            if (strcmp(prop->name, MENU_DATA_PROP) == 0) {
                update_menu(ctx, prop->data);
            }
            break;
        default:
            break;
    }
}

// handle client message event
static void handle_client_message(mpv_event *event) {
    mpv_event_client_message *msg = event->data;
    if (msg->num_args < 1) return;

    const char *cmd = msg->args[0];
    if (strcmp(cmd, "show") == 0) {
        PostMessageW(ctx->hwnd, WM_SHOWMENU, 0, 0);
    } else if (msg->num_args > 1) {
        if (strcmp(cmd, "clipboard/get") == 0) {
            char *text = get_clipboard(NULL);
            if (text == NULL) return;

            mpv_command(ctx->mpv,
                        (const char *[]){"script-message-to", msg->args[1],
                                         "clipboard-get-reply", text, NULL});
            talloc_free(text);
        } else if (strcmp(cmd, "clipboard/set") == 0) {
            set_clipboard(msg->args[1]);
        } else if (strcmp(cmd, "dialog/open") == 0) {
            char *path = open_dialog(NULL, ctx);
            if (path == NULL) return;

            mpv_command(ctx->mpv,
                        (const char *[]){"script-message-to", msg->args[1],
                                         "dialog-open-reply", path, NULL});
            talloc_free(path);
        } else if (strcmp(cmd, "dialog/open-multi") == 0) {
            void *tmp = talloc_new(NULL);
            char **paths = open_dialog_multi(tmp, ctx);
            if (paths == NULL) {
                talloc_free(tmp);
                return;
            }

            int count = 0;
            while (paths[count] != NULL) count++;

            char **args = talloc_array(tmp, char *, count + 4);
            args[0] = talloc_strdup(tmp, "script-message-to");
            args[1] = talloc_strdup(tmp, msg->args[1]);
            args[2] = talloc_strdup(tmp, "dialog-open-multi-reply");
            for (int i = 0; i < count; i++) args[i + 3] = paths[i];
            args[count + 3] = NULL;

            mpv_command(ctx->mpv, (const char **)args);
            talloc_free(tmp);
        } else if (strcmp(cmd, "dialog/open-folder") == 0) {
            char *path = open_folder(NULL, ctx);
            if (path == NULL) return;

            mpv_command(ctx->mpv, (const char *[]){
                                      "script-message-to", msg->args[1],
                                      "dialog-open-folder-reply", path, NULL});
            talloc_free(path);
        } else if (strcmp(cmd, "dialog/save") == 0) {
            char *path = save_dialog(NULL, ctx);
            if (path == NULL) return;

            mpv_command(ctx->mpv,
                        (const char *[]){"script-message-to", msg->args[1],
                                         "dialog-save-reply", path, NULL});
            talloc_free(path);
        }
    }
}

// create and init plugin context
static void create_plugin_ctx(mpv_handle *mpv) {
    ctx = talloc_zero(NULL, plugin_ctx);
    ctx->hmenu = CreatePopupMenu();
    ctx->hmenu_ctx = talloc_new(ctx);
    ctx->mpv = mpv;

    ctx->dispatch = mp_dispatch_create(ctx);
}

// destroy plugin context and free memory
static void destroy_plugin_ctx() {
    if (ctx->hmenu) DestroyMenu(ctx->hmenu);
    if (ctx->hwnd && ctx->wnd_proc)
        SetWindowLongPtrW(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)ctx->wnd_proc);
    talloc_free(ctx);
}

// entry point of plugin
MPV_EXPORT int mpv_open_cplugin(mpv_handle *handle) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    create_plugin_ctx(handle);

    mpv_node node = {0};
    if (mpv_get_property(handle, MENU_DATA_PROP, MPV_FORMAT_NODE, &node) >= 0)
        update_menu(ctx, &node);

    mpv_observe_property(handle, 0, "window-id", MPV_FORMAT_INT64);
    mpv_observe_property(handle, 0, MENU_DATA_PROP, MPV_FORMAT_NODE);

    mpv_command(handle, (const char *[]){"script-message", "menu-init",
                                         mpv_client_name(handle), NULL});

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
            default:
                break;
        }
    }

    mpv_unobserve_property(handle, 0);
    destroy_plugin_ctx();

    CoUninitialize();

    return 0;
}

// convert utf8 string to wchar_t
wchar_t *mp_from_utf8(void *talloc_ctx, const char *s) {
    int count = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (count <= 0) abort();
    wchar_t *ret = talloc_array(talloc_ctx, wchar_t, count);
    MultiByteToWideChar(CP_UTF8, 0, s, -1, ret, count);
    return ret;
}

// convert wchar_t string to utf8
char *mp_to_utf8(void *talloc_ctx, const wchar_t *s) {
    int count = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
    if (count <= 0) abort();
    char *ret = talloc_array(talloc_ctx, char, count);
    WideCharToMultiByte(CP_UTF8, 0, s, -1, ret, count, NULL, NULL);
    return ret;
}

static void async_cmd_fn(void *data) {
    mpv_command_string(ctx->mpv, (const char *)data);
}

// run command on none-ui thread
void mp_command_async(const char *args) {
    mp_dispatch_enqueue(ctx->dispatch, async_cmd_fn, (void *)args);
    mpv_wakeup(ctx->mpv);
}
