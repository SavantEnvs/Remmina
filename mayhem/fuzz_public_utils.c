/* Fuzz Remmina's string-handling helpers: server:port parsing, resolution parsing, search/replace,
 * GString utilities and the semicolon-separated string array. Byte 0 selects the helper; the rest
 * is split on '\n' into up to three string arguments. */
#include <glib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "remmina_public.h"
#include "remmina_utils.h"
#include "remmina_string_array.h"

static char *take_line(const uint8_t **p, const uint8_t *end)
{
	const uint8_t *s = *p;
	const uint8_t *nl = memchr(s, '\n', end - s);
	const uint8_t *e = nl ? nl : end;
	char *r = g_strndup((const char *)s, e - s);
	*p = nl ? nl + 1 : end;
	return r;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 1)
		return -1;
	uint8_t mode = data[0] % 8;
	const uint8_t *p = data + 1, *end = data + size;
	char *a = take_line(&p, end);
	char *b = take_line(&p, end);
	char *c = take_line(&p, end);

	switch (mode) {
	case 0: {
		gchar *host = NULL;
		gint port = 0;
		remmina_public_get_server_port(a, 3389, &host, &port);
		g_free(host);
		break;
	}
	case 1: {
		gchar *host = NULL;
		gint port = 0;
		remmina_public_get_server_port_old(a, 5900, &host, &port);
		g_free(host);
		break;
	}
	case 2: {
		int w = 0, h = 0;
		remmina_public_split_resolution_string(a, &w, &h);
		break;
	}
	case 3: {
		if (*b) {
			gchar *r = remmina_public_str_replace(a, b, c);
			g_free(r);
		}
		break;
	}
	case 4: {
		GString *s = g_string_new(a);
		if (*b)
			remmina_utils_string_replace_all(s, b, c);
		g_string_free(s, TRUE);
		break;
	}
	case 5: {
		GString *s = g_string_new(a);
		/* remmina_utils_string_find() requires a non-empty needle (g_return_val_if_fail(!EMPTY(needle))). */
		if (*b) {
			gint pos = remmina_utils_string_find(s, (gint)strlen(c) % 8, -1, b);
			if (pos >= 0)
				remmina_utils_string_replace(s, pos, (gint)strlen(b), c);
		}
		g_string_free(s, TRUE);
		break;
	}
	case 6: {
		gchar *r = remmina_utils_string_strip(a);
		g_free(r);
		break;
	}
	default: {
		RemminaStringArray *arr = remmina_string_array_new_from_string(a);
		remmina_string_array_add(arr, b);
		remmina_string_array_remove(arr, c);
		remmina_string_array_sort(arr);
		remmina_string_array_intersect(arr, b);
		gchar *r = remmina_string_array_to_string(arr);
		g_free(r);
		remmina_string_array_free(arr);
		break;
	}
	}
	g_free(a);
	g_free(b);
	g_free(c);
	return 0;
}
