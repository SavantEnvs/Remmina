# GIOChannel leak in `remmina_spice_file_import()`

Same as the RDP importer: the `GIOChannel` from `g_io_channel_new_file()` is never shut down or unrefed.
The harness calls `remmina_spice_file_import_channel()` directly and releases the channel itself.

Fix: `g_io_channel_shutdown(channel, TRUE, NULL); g_io_channel_unref(channel);` before returning.
