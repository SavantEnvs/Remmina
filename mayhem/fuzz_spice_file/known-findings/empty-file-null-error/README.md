# NULL `error->message` on an empty `.vv` file

Same defect as the RDP importer: `plugins/spice/spice_file.c:109` dereferences `error->message` after
`g_io_channel_read_chars()` returns `G_IO_STATUS_EOF` with no `GError` set.

Impact: importing an empty file crashes the client.
Fix: treat EOF separately, or print `error ? error->message : "EOF"`.

The harness rejects empty input (`size == 0` returns -1), so **Mayhem will never report this bug**.
libFuzzer always executes the empty input before the corpus, and a crash there fails Mayhem's `-runs=5`
startup probe, which would fail every run. Every non-empty input is delivered unfiltered, including
inputs, including one-byte files.

Reproduce: `/mayhem/fuzz_spice_file-standalone` rejects empty input too; call `remmina_spice_file_import_channel()` on `empty.vv` directly (e.g. from a scratch harness without the size check).
