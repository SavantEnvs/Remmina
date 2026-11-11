/* Fuzz Remmina's connection-profile (.remmina) loader: GKeyFile parse, protocol lookup, encrypted
 * password decryption, resolution splitting, ssh-key upgrade, and the typed getters. */
#include <glib.h>
#include <glib/gstdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "remmina_fuzz_init.h"
#include "remmina_file.h"

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
	remmina_fuzz_init();
	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	const char *tmpdir = getenv("TMPDIR");
	char path[4096];
	snprintf(path, sizeof(path), "%s/remmina-fuzz-XXXXXX.remmina", tmpdir && *tmpdir ? tmpdir : "/tmp");
	int fd = mkstemps(path, 8);
	if (fd < 0)
		return -1;
	size_t off = 0;
	while (off < size) {
		ssize_t n = write(fd, data + off, size - off);
		if (n <= 0)
			break;
		off += (size_t)n;
	}
	close(fd);

	RemminaFile *rf = remmina_file_load(path);
	if (rf) {
		(void)remmina_file_get_string(rf, "server");
		(void)remmina_file_get_string(rf, "password");
		(void)remmina_file_get_string(rf, "resolution_width");
		(void)remmina_file_get_int(rf, "colordepth", 8);
		(void)remmina_file_get_double(rf, "scale", 1.0);
		(void)remmina_file_get_int(rf, "ssh_tunnel_enabled", 0);
		(void)remmina_file_get_string(rf, "protocol");
		(void)remmina_file_get_icon_name(rf);
		g_free(remmina_file_get_datetime(rf));
		remmina_file_free(rf);
	}
	g_unlink(path);
	return rf ? 0 : -1;
}
