#include <windows.h>
#include "mpv_talloc.h"
#include "clipboard.h"

// get clipboard text, always return utf8 string
char *get_clipboard(plugin_ctx *ctx, void *talloc_ctx) {
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
void set_clipboard(plugin_ctx *ctx, const char *text) {
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
