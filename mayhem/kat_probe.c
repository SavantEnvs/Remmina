/* Known-answer probe for the Remmina core helpers. Linked against the normally-built core objects
 * by build.sh; mayhem/test.sh runs it and compares every line against the expected value. */
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "remmina_fuzz_init.h"
#include "remmina_crypt.h"
#include "remmina_file.h"
#include "remmina_public.h"
#include "remmina_utils.h"
#include "remmina_string_array.h"

static void out(const char *id, const char *val)
{
	printf("KAT %s=%s\n", id, val ? val : "(null)");
}

static void server_port(const char *id, const char *server, gint defport)
{
	gchar *host = NULL;
	gint port = -1;
	remmina_public_get_server_port(server, defport, &host, &port);
	gchar *s = g_strdup_printf("%s|%d", host ? host : "(null)", port);
	out(id, s);
	g_free(s);
	g_free(host);
}

int main(void)
{
	remmina_fuzz_init();

	server_port("server_port.host_port", "rdp.example.com:3390", 3389);
	server_port("server_port.default", "example.org", 3389);
	server_port("server_port.ipv6", "[::1]:5901", 3389);

	int w = 0, h = 0;
	int ok = remmina_public_split_resolution_string("1920x1080", &w, &h);
	gchar *s = g_strdup_printf("%d|%d|%d", ok, w, h);
	out("resolution.ok", s);
	g_free(s);
	w = h = 0;
	s = g_strdup_printf("%d", remmina_public_split_resolution_string("wide", &w, &h));
	out("resolution.bad", s);
	g_free(s);

	s = remmina_public_str_replace("a-b-c", "-", "+");
	out("str_replace", s);
	g_free(s);

	GString *gs = g_string_new("foo bar foo");
	guint n = remmina_utils_string_replace_all(gs, "foo", "bazz");
	s = g_strdup_printf("%s|%u", gs->str, n);
	out("replace_all", s);
	g_free(s);
	g_string_free(gs, TRUE);

	s = remmina_utils_string_strip("\"a\tb\n\"");
	out("strip", s);
	g_free(s);

	RemminaStringArray *arr = remmina_string_array_new_from_string("pear,apple,fig");
	s = g_strdup_printf("%d", remmina_string_array_find(arr, "apple"));
	out("string_array.find", s);
	g_free(s);
	remmina_string_array_remove(arr, "pear");
	s = remmina_string_array_to_string(arr);
	out("string_array.after_remove", s);
	g_free(s);
	remmina_string_array_free(arr);

	gchar *enc = remmina_crypt_encrypt("hunter2");
	out("crypt.encrypt", enc);
	gchar *dec = remmina_crypt_decrypt(enc);
	out("crypt.roundtrip", dec);
	g_free(dec);

	char path[] = "/tmp/remmina-kat-XXXXXX.remmina";
	int fd = mkstemps(path, 8);
	if (fd < 0)
		return 2;
	gchar *content = g_strdup_printf("[remmina]\nname=Test\nprotocol=RDP\nserver=10.0.0.5:3390\n"
					 "resolution=1280x800\nssh_enabled=0\npassword=%s\n", enc);
	if (write(fd, content, strlen(content)) < 0)
		return 2;
	close(fd);
	RemminaFile *rf = remmina_file_load(path);
	if (rf) {
		out("file.name", remmina_file_get_string(rf, "name"));
		out("file.server", remmina_file_get_string(rf, "server"));
		out("file.width", remmina_file_get_string(rf, "resolution_width"));
		out("file.height", remmina_file_get_string(rf, "resolution_height"));
		out("file.password", remmina_file_get_string(rf, "password"));
		remmina_file_free(rf);
	} else {
		out("file.name", NULL);
	}
	g_unlink(path);
	g_free(content);
	g_free(enc);
	return 0;
}
