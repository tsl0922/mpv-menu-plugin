#ifndef MP_MISC_NODE_H_
#define MP_MISC_NODE_H_

#include "mpv/client.h"
#include "misc/bstr.h"

void node_init(struct mpv_node *dst, int format, struct mpv_node *parent);
struct mpv_node *node_array_add(struct mpv_node *dst, int format);
struct mpv_node *node_map_add(struct mpv_node *dst, const char *key, int format);
struct mpv_node *node_map_badd(struct mpv_node *dst, struct bstr key, int format);
void node_map_add_string(struct mpv_node *dst, const char *key, const char *val);
void node_map_add_int64(struct mpv_node *dst, const char *key, int64_t v);
void node_map_add_double(struct mpv_node *dst, const char *key, double v);
void node_map_add_flag(struct mpv_node *dst, const char *key, bool v);
mpv_node *node_map_get(mpv_node *src, const char *key);
mpv_node *node_map_bget(mpv_node *src, struct bstr key);
bool equal_mpv_value(const void *a, const void *b, mpv_format format);
bool equal_mpv_node(const struct mpv_node *a, const struct mpv_node *b);
void copy_mpv_node(struct mpv_node *dst, struct mpv_node *src);

#endif
