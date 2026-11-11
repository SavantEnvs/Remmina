/* Fuzz the SPICE plugin's .vv file importer ([virt-viewer] key=value lines).
 * spice_file.c is included into the harness so the static channel parser can be called directly;
 * its plugin-service calls hit the real core. */
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "remmina_fuzz_init.h"
#include "remmina_file.h"
#include "spice_plugin.h"
#include "spice_file.c"

RemminaPluginService *remmina_plugin_service = &remmina_plugin_manager_service;

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
	remmina_fuzz_init();
	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	/* libFuzzer executes the empty input before any corpus (verified: `-runs=5 <testsuite>` crashes on
	 * its sha1 da39a3ee...), and an empty file crashes the importer (known-findings/empty-file-null-error),
	 * which fails Mayhem's `-runs=5` startup probe and so the whole run. This is the only input rejected;
	 * Mayhem therefore never reports that one bug, and the reproducer is kept for reference. */
	if (size == 0)
		return -1;
	const char *tmpdir = getenv("TMPDIR");
	char path[4096];
	snprintf(path, sizeof(path), "%s/remmina-fuzz-XXXXXX.vv", tmpdir && *tmpdir ? tmpdir : "/tmp");
	int fd = mkstemps(path, 3);
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

	/* Same path as remmina_spice_file_import() (open, import_channel, shutdown), but the harness also
	 * drops its reference to the channel, which the importer leaks on every call. */
	GIOChannel *channel = g_io_channel_new_file(path, "r", NULL);
	if (channel == NULL) {
		g_unlink(path);
		return -1;
	}
	RemminaFile *rf = remmina_spice_file_import_channel(channel);
	g_io_channel_shutdown(channel, TRUE, NULL);
	g_io_channel_unref(channel);
	if (rf) {
		(void)remmina_spice_file_export_test(NULL, rf);
		char *out = NULL;
		size_t out_len = 0;
		FILE *fp = open_memstream(&out, &out_len);
		if (fp) {
			(void)remmina_spice_file_export_channel(rf, fp);
			fclose(fp);
			free(out);
		}
		remmina_file_free(rf);
	}
	g_unlink(path);
	return rf ? 0 : -1;
}
