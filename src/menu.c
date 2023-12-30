// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <windows.h>
#include <mpv/client.h>
#include "misc/bstr.h"
#include "misc/dispatch.h"

typedef struct {
    mpv_handle *mpv;              // mpv client handle
    char *prop;                   // property name
    void *alloc_ctx;              // menu alloc context
    mp_dispatch_queue *dispatch;  // dispatch queue

    HWND hwnd;         // window handle
    HMENU hmenu;       // menu handle
    WNDPROC wnd_proc;  // previous window procedure
} plugin_ctx;

// global plugin context
plugin_ctx *ctx = NULL;

#define WM_SHOWMENU (WM_USER + 1)
#define PROP_NAME "user-data/menu/items"

wchar_t *mp_from_utf8(void *talloc_ctx, const char *s) {
    int count = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (count <= 0) abort();
    wchar_t *ret = talloc_array(talloc_ctx, wchar_t, count);
    MultiByteToWideChar(CP_UTF8, 0, s, -1, ret, count);
    return ret;
}

// escape & to && for menu title
static wchar_t *escape_title(void *talloc_ctx, char *title) {
    void *tmp = talloc_new(NULL);
    bstr left, rest;
    bstr escaped = bstr0(NULL);

    left = bstr_split(bstr0(title), "&", &rest);
    while (rest.len > 0) {
        bstr_xappend(tmp, &escaped, left);
        bstr_xappend(tmp, &escaped, bstr0("&&"));
        left = bstr_split(rest, "&", &rest);
    }
    bstr_xappend(tmp, &escaped, left);

    wchar_t *ret = mp_from_utf8(talloc_ctx, bstrdup0(tmp, escaped));
    talloc_free(tmp);
    return ret;
}

static void async_cmd_fn(void *data) {
    mpv_command_string(ctx->mpv, (const char *)data);
}

static void mp_command_async(const char *args) {
    mp_dispatch_enqueue(ctx->dispatch, async_cmd_fn, (void *)args);
    mpv_wakeup(ctx->mpv);
}

static int append_menu(HMENU hmenu, UINT fMask, UINT fType, UINT fState,
                       wchar_t *title, HMENU submenu, void *data) {
    static UINT id = WM_USER + 100;
    MENUITEMINFOW mii = {0};

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | fMask;
    mii.wID = id++;

    if (fMask & MIIM_FTYPE) mii.fType = fType;
    if (fMask & MIIM_STATE) mii.fState = fState;
    if (fMask & MIIM_STRING) {
        mii.dwTypeData = title;
        mii.cch = wcslen(title);
    }
    if (fMask & MIIM_SUBMENU) mii.hSubMenu = submenu;
    if (fMask & MIIM_DATA) mii.dwItemData = (ULONG_PTR)data;

    return InsertMenuItemW(hmenu, -1, TRUE, &mii) ? mii.wID : -1;
}

void show_menu(POINT *pt) {
    RECT rc;
    GetClientRect(ctx->hwnd, &rc);
    ScreenToClient(ctx->hwnd, pt);
    if (!PtInRect(&rc, *pt)) return;

    ClientToScreen(ctx->hwnd, pt);
    TrackPopupMenuEx(ctx->hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt->x, pt->y,
                     ctx->hwnd, NULL);
}

void handle_menu(int id) {
    MENUITEMINFOW mii = {0};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA;
    if (!GetMenuItemInfoW(ctx->hmenu, id, FALSE, &mii)) return;

    if (mii.dwItemData) mp_command_async((const char *)mii.dwItemData);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    POINT pt;

    switch (uMsg) {
        case WM_SHOWMENU:
            if (GetCursorPos(&pt)) show_menu(&pt);
            break;
        case WM_COMMAND:
            handle_menu(LOWORD(wParam));
            break;
        default:
            break;
    }

    return CallWindowProcW(ctx->wnd_proc, hWnd, uMsg, wParam, lParam);
}

static UINT parse_state(mpv_node *node) {
    UINT fState = 0;
    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node *item = &node->u.list->values[i];
        if (item->format != MPV_FORMAT_STRING) continue;

        if (strcmp(item->u.string, "checked") == 0) {
            fState |= MFS_CHECKED;
        } else if (strcmp(item->u.string, "disabled") == 0) {
            fState |= MFS_DISABLED;
        }
    }
    return fState;
}

// data structure:
//
// MPV_FORMAT_NODE_ARRAY
//   MPV_FORMAT_NODE_MAP (menu item)
//      "type"           MPV_FORMAT_STRING
//      "title"          MPV_FORMAT_STRING
//      "cmd"            MPV_FORMAT_STRING
//      "state"          MPV_FORMAT_NODE_ARRAY[MPV_FORMAT_STRING]
//      "submenu"        MPV_FORMAT_NODE_ARRAY[menu item]
static void parse_menu(HMENU hmenu, mpv_node *node) {
    if (node->format != MPV_FORMAT_NODE_ARRAY || node->u.list->num == 0) return;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node *item = &node->u.list->values[i];
        if (item->format != MPV_FORMAT_NODE_MAP) continue;

        mpv_node_list *items = item->u.list;

        char *type = "";
        char *title = NULL;
        char *cmd = NULL;
        UINT fState = 0;
        HMENU submenu = NULL;

        for (int j = 0; j < items->num; j++) {
            char *key = items->keys[j];
            mpv_node *value = &items->values[j];

            switch (value->format) {
                case MPV_FORMAT_STRING:
                    if (strcmp(key, "title") == 0) {
                        title = value->u.string;
                    } else if (strcmp(key, "cmd") == 0) {
                        cmd = value->u.string;
                    } else if (strcmp(key, "type") == 0) {
                        type = value->u.string;
                    }
                    break;
                case MPV_FORMAT_NODE_ARRAY:
                    if (strcmp(key, "state") == 0) {
                        fState = parse_state(value);
                    } else if (strcmp(key, "submenu") == 0) {
                        submenu = CreatePopupMenu();
                        parse_menu(submenu, value);
                    }
                    break;
            }
        }

        if (strcmp(type, "separator") == 0) {
            append_menu(hmenu, MIIM_FTYPE, MFT_SEPARATOR, 0, NULL, NULL, NULL);
        } else {
            if (title == NULL || strlen(title) == 0) continue;

            UINT fMask = MIIM_STRING | MIIM_STATE;
            bool grayed = false;
            if (strcmp(type, "submenu") == 0) {
                if (submenu == NULL) submenu = CreatePopupMenu();
                fMask |= MIIM_SUBMENU;
                grayed = GetMenuItemCount(submenu) == 0;
            } else {
                fMask |= MIIM_DATA;
                grayed = cmd == NULL || cmd[0] == '\0' || cmd[0] == '#' ||
                         strcmp(cmd, "ignore") == 0;
            }
            int id = append_menu(hmenu, fMask, 0, fState,
                                 escape_title(ctx->alloc_ctx, title), submenu,
                                 talloc_strdup(ctx->alloc_ctx, cmd));
            if (grayed) EnableMenuItem(hmenu, id, MF_BYCOMMAND | MF_GRAYED);
        }
    }
}

static void update_menu(mpv_node *node) {
    // clear menu
    while (GetMenuItemCount(ctx->hmenu) > 0)
        RemoveMenu(ctx->hmenu, 0, MF_BYPOSITION);
    talloc_free_children(ctx->alloc_ctx);

    // parse menu
    parse_menu(ctx->hmenu, node);
}

static void handle_property_change(mpv_event *event) {
    mpv_event_property *prop = event->data;
    switch (prop->format) {
        case MPV_FORMAT_INT64:
            if (strcmp(prop->name, "window-id") == 0) {
                int64_t wid = *(int64_t *)prop->data;
                if (wid > 0) {
                    ctx->hwnd = (HWND)wid;
                    ctx->wnd_proc = (WNDPROC)SetWindowLongPtrW(
                        ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
                }
            }
            break;
        case MPV_FORMAT_NODE:
            if (strcmp(prop->name, PROP_NAME) == 0) {
                update_menu((mpv_node *)prop->data);
            }
            break;
        default:
            break;
    }
}

static void handle_client_message(mpv_event *event) {
    mpv_event_client_message *msg = event->data;
    if (msg->num_args < 1) return;

    const char *cmd = msg->args[0];
    if (strcmp(cmd, "show") == 0) PostMessageW(ctx->hwnd, WM_SHOWMENU, 0, 0);
}

MPV_EXPORT int mpv_open_cplugin(mpv_handle *handle) {
    ctx = talloc_zero(NULL, plugin_ctx);
    ctx->mpv = handle;
    ctx->hmenu = CreatePopupMenu();
    ctx->alloc_ctx = talloc_new(ctx);
    ctx->dispatch = mp_dispatch_create(ctx);

    // the lua plugin may started before this plugin
    mpv_node node = {0};
    if (mpv_get_property(handle, PROP_NAME, MPV_FORMAT_NODE, &node) >= 0) {
        update_menu(&node);
        mpv_free_node_contents(&node);
    }

    mpv_observe_property(handle, 0, "window-id", MPV_FORMAT_INT64);
    mpv_observe_property(handle, 0, PROP_NAME, MPV_FORMAT_NODE);

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

    if (ctx->hwnd && ctx->wnd_proc)
        SetWindowLongPtrW(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)ctx->wnd_proc);
    DestroyMenu(ctx->hmenu);
    talloc_free(ctx);

    return 0;
}
