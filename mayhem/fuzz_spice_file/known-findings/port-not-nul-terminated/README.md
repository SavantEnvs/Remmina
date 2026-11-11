# Stack buffer over-read in the SPICE `.vv` importer

`repro.vv` has `port=1234567890`. `remmina_spice_file_import_field()` copies the port into a fixed
`port` buffer without guaranteeing a NUL terminator, and `plugins/spice/spice_file.c:155` then runs
`strlen(port)` / `strncat` on it. ASan reports a stack-buffer-overflow.

Impact: a crafted virt-viewer file overruns a stack buffer when imported.
Fix: bound the copy to `sizeof(port) - 1` and NUL-terminate.

Reproduce: `/mayhem/fuzz_spice_file-standalone mayhem/fuzz_spice_file/known-findings/port-not-nul-terminated/repro.vv`
