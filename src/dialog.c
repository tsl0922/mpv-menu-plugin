#include <windows.h>
#include <shobjidl.h>
#include <mpv/client.h>
#include "mpv_talloc.h"
#include "dialog.h"

#define DIALOG_FILTER_PROP "user-data/menu/dialog/filters"
#define DIALOG_DEF_PATH_PROP "user-data/menu/dialog/default-path"
#define DIALOG_DEF_NAME_PROP "user-data/menu/dialog/default-name"

// add filters to dialog from user property, default to first one
static void add_filters(mpv_handle *mpv, IFileDialog *pfd) {
    mpv_node node = {0};
    if (mpv_get_property(mpv, DIALOG_FILTER_PROP, MPV_FORMAT_NODE, &node) < 0)
        return;
    if (node.format != MPV_FORMAT_NODE_ARRAY) goto done;

    void *tmp = talloc_new(NULL);
    mpv_node_list *list = node.u.list;
    COMDLG_FILTERSPEC *specs = talloc_array(tmp, COMDLG_FILTERSPEC, list->num);
    UINT count = 0;

    for (int i = 0; i < list->num; i++) {
        mpv_node *item = &list->values[i];
        if (item->format != MPV_FORMAT_NODE_MAP) continue;

        char *name = NULL, *spec = NULL;
        mpv_node_list *values = item->u.list;

        for (int j = 0; j < values->num; j++) {
            char *key = values->keys[j];
            mpv_node *value = &values->values[j];
            if (value->format != MPV_FORMAT_STRING) continue;

            if (strcmp(key, "name") == 0) name = value->u.string;
            if (strcmp(key, "spec") == 0) spec = value->u.string;
        }

        if (name != NULL && spec != NULL) {
            specs[count].pszName = mp_from_utf8(tmp, name);
            specs[count].pszSpec = mp_from_utf8(tmp, spec);
            count++;
        }
    }

    if (count > 0) {
        pfd->lpVtbl->SetFileTypes(pfd, count, specs);
        pfd->lpVtbl->SetFileTypeIndex(pfd, 1);
        pfd->lpVtbl->SetDefaultExtension(pfd, specs[0].pszSpec);
    }

    talloc_free(tmp);

done:
    mpv_free_node_contents(&node);
}

// set default path from user property
static void set_default_path(mpv_handle *mpv, IFileDialog *pfd) {
    mpv_node node = {0};
    if (mpv_get_property(mpv, DIALOG_DEF_PATH_PROP, MPV_FORMAT_NODE, &node) < 0)
        return;

    IShellItem *folder;
    wchar_t *w_path = mp_from_utf8(NULL, node.u.string);

    if (SUCCEEDED(SHCreateItemFromParsingName(w_path, NULL, &IID_IShellItem,
                                              (void **)&folder)))
        pfd->lpVtbl->SetDefaultFolder(pfd, folder);

    talloc_free(w_path);
    mpv_free_node_contents(&node);
}

// set default name used for save dialog
static void set_default_name(mpv_handle *mpv, IFileDialog *pfd) {
    mpv_node node = {0};
    if (mpv_get_property(mpv, DIALOG_DEF_NAME_PROP, MPV_FORMAT_NODE, &node) < 0)
        return;

    wchar_t *w_name = mp_from_utf8(NULL, node.u.string);
    pfd->lpVtbl->SetFileName(pfd, w_name);

    talloc_free(w_name);
    mpv_free_node_contents(&node);
}

// add options to dialog, previous options are preserved
static void add_options(IFileDialog *pfd, DWORD options) {
    DWORD dwOptions;
    if (SUCCEEDED(pfd->lpVtbl->GetOptions(pfd, &dwOptions))) {
        pfd->lpVtbl->SetOptions(pfd, dwOptions | options);
    }
}

// single file open dialog
char *open_dialog(void *talloc_ctx, plugin_ctx *ctx) {
    IFileOpenDialog *pfd = NULL;
    if (FAILED(CoCreateInstance(&CLSID_FileOpenDialog, NULL,
                                CLSCTX_INPROC_SERVER, &IID_IFileDialog,
                                (void **)&pfd)))
        return NULL;

    add_filters(ctx->mpv, (IFileDialog *)pfd);
    set_default_path(ctx->mpv, (IFileDialog *)pfd);
    add_options((IFileDialog *)pfd, FOS_FORCEFILESYSTEM);

    char *path = NULL;

    if (SUCCEEDED(pfd->lpVtbl->Show(pfd, ctx->hwnd))) {
        IShellItem *psi;
        if (SUCCEEDED(pfd->lpVtbl->GetResult(pfd, &psi))) {
            wchar_t *w_path;
            if (SUCCEEDED(psi->lpVtbl->GetDisplayName(psi, SIGDN_FILESYSPATH,
                                                      &w_path))) {
                path = mp_to_utf8(talloc_ctx, w_path);
                CoTaskMemFree(w_path);
            }
            psi->lpVtbl->Release(psi);
        }
    }

    pfd->lpVtbl->Release(pfd);

    return path;
}

// multiple file open dialog
char **open_dialog_multi(void *talloc_ctx, plugin_ctx *ctx) {
    IFileOpenDialog *pfd = NULL;
    if (FAILED(CoCreateInstance(&CLSID_FileOpenDialog, NULL,
                                CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog,
                                (void **)&pfd)))
        return NULL;

    add_filters(ctx->mpv, (IFileDialog *)pfd);
    set_default_path(ctx->mpv, (IFileDialog *)pfd);
    add_options((IFileDialog *)pfd, FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT);

    char **paths = NULL;

    if (SUCCEEDED(pfd->lpVtbl->Show(pfd, ctx->hwnd))) {
        IShellItemArray *psia;
        if (SUCCEEDED(pfd->lpVtbl->GetResults(pfd, &psia))) {
            DWORD count;
            if (SUCCEEDED(psia->lpVtbl->GetCount(psia, &count))) {
                paths =
                    talloc_zero_size(talloc_ctx, sizeof(char *) * (count + 1));
                for (DWORD i = 0; i < count; i++) {
                    IShellItem *psi;
                    if (SUCCEEDED(psia->lpVtbl->GetItemAt(psia, i, &psi))) {
                        wchar_t *w_path;
                        if (SUCCEEDED(psi->lpVtbl->GetDisplayName(
                                psi, SIGDN_FILESYSPATH, &w_path))) {
                            paths[i] = mp_to_utf8(talloc_ctx, w_path);
                            CoTaskMemFree(w_path);
                        }
                        psi->lpVtbl->Release(psi);
                    }
                }
            }
            psia->lpVtbl->Release(psia);
        }
    }

    pfd->lpVtbl->Release(pfd);

    return paths;
}

// folder open dialog
char *open_folder(void *talloc_ctx, plugin_ctx *ctx) {
    IFileOpenDialog *pfd = NULL;
    if (FAILED(CoCreateInstance(&CLSID_FileOpenDialog, NULL,
                                CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog,
                                (void **)&pfd)))
        return NULL;

    set_default_path(ctx->mpv, (IFileDialog *)pfd);
    add_options((IFileDialog *)pfd, FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS);

    char *path = NULL;

    if (SUCCEEDED(pfd->lpVtbl->Show(pfd, ctx->hwnd))) {
        IShellItem *psi;
        if (SUCCEEDED(pfd->lpVtbl->GetResult(pfd, &psi))) {
            wchar_t *w_path;
            if (SUCCEEDED(psi->lpVtbl->GetDisplayName(psi, SIGDN_FILESYSPATH,
                                                      &w_path))) {
                path = mp_to_utf8(talloc_ctx, w_path);
                CoTaskMemFree(w_path);
            }
            psi->lpVtbl->Release(psi);
        }
    }

    pfd->lpVtbl->Release(pfd);

    return path;
}

// save dialog
char *save_dialog(void *talloc_ctx, plugin_ctx *ctx) {
    IFileSaveDialog *pfd = NULL;
    if (FAILED(CoCreateInstance(&CLSID_FileSaveDialog, NULL,
                                CLSCTX_INPROC_SERVER, &IID_IFileSaveDialog,
                                (void **)&pfd)))
        return NULL;

    add_filters(ctx->mpv, (IFileDialog *)pfd);
    set_default_path(ctx->mpv, (IFileDialog *)pfd);
    set_default_name(ctx->mpv, (IFileDialog *)pfd);
    add_options((IFileDialog *)pfd, FOS_FORCEFILESYSTEM);

    char *path = NULL;

    if (SUCCEEDED(pfd->lpVtbl->Show(pfd, ctx->hwnd))) {
        IShellItem *psi;
        if (SUCCEEDED(pfd->lpVtbl->GetResult(pfd, &psi))) {
            wchar_t *w_path;
            if (SUCCEEDED(psi->lpVtbl->GetDisplayName(psi, SIGDN_FILESYSPATH,
                                                      &w_path))) {
                path = mp_to_utf8(talloc_ctx, w_path);
                CoTaskMemFree(w_path);
            }
            psi->lpVtbl->Release(psi);
        }
    }

    pfd->lpVtbl->Release(pfd);

    return path;
}