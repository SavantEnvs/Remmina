/* Shared one-time setup for the Remmina harnesses: the core's plugin table must exist and
 * remmina_pref.secret must be a valid 3DES key+IV (32+ bytes, base64) for password decryption. */
#ifndef REMMINA_FUZZ_INIT_H
#define REMMINA_FUZZ_INIT_H

#include <glib.h>
#include "remmina/plugin.h"
#include "remmina_plugin_manager.h"
#include "remmina_pref.h"
#include "remmina_masterthread_exec.h"

static const RemminaProtocolSetting fuzz_basic_settings[] = {
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER, NULL, NULL, FALSE, NULL, NULL, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "password", "Password", FALSE, NULL, NULL, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL, NULL, NULL }
};

/* The protocol names real profiles carry; each gets a stand-in so the loader's protocol lookup and
 * its encrypted-setting handling (password type, ssh_tunnel_password, ...) are reachable for all. */
static const char *const fuzz_protocol_names[] = { "RDP", "VNC", "SSH", "SFTP", "SPICE", "X2GO", "NX", "WWW" };
#define FUZZ_NPROTOCOLS (sizeof(fuzz_protocol_names) / sizeof(fuzz_protocol_names[0]))
static RemminaProtocolPlugin fuzz_protocol_plugins[FUZZ_NPROTOCOLS];

static void fuzz_log_discard(const gchar *d, GLogLevelFlags l, const gchar *m, gpointer u) {}
static void fuzz_print_discard(const gchar *s) {}

static void remmina_fuzz_init(void)
{
	g_log_set_default_handler(fuzz_log_discard, NULL);
	g_set_print_handler(fuzz_print_discard);
	remmina_masterthread_exec_save_main_thread_id();
	remmina_pref.secret = g_strdup("AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4v");
	remmina_plugin_manager_init();
	for (size_t i = 0; i < FUZZ_NPROTOCOLS; i++) {
		fuzz_protocol_plugins[i] = (RemminaProtocolPlugin){
			.type = REMMINA_PLUGIN_TYPE_PROTOCOL,
			.name = fuzz_protocol_names[i],
			.description = "fuzz stand-in for a protocol plugin",
			.domain = "remmina",
			.version = "0",
			.icon_name = "remmina-fuzz",
			.icon_name_ssh = "remmina-fuzz-ssh",
			.basic_settings = fuzz_basic_settings,
		};
		remmina_plugin_manager_service.register_plugin((RemminaPlugin *)&fuzz_protocol_plugins[i]);
	}
}

#endif
