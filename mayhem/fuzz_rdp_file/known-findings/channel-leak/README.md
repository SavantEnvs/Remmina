# GIOChannel leak in `remmina_rdp_file_import()`

`remmina_rdp_file_import()` opens a `GIOChannel` with `g_io_channel_new_file()` and never shuts down or
unrefs it, leaking the channel and its buffers on every import. Leak detection is off in this fleet
(`lsan_off.cc`), but the leak grows without bound in-process and OOMs libFuzzer within a minute.
The harness therefore calls `remmina_rdp_file_import_channel()` directly and releases the channel itself.

Fix: `g_io_channel_shutdown(channel, TRUE, NULL); g_io_channel_unref(channel);` before returning.
