# NULL `error->message` on an empty `.rdp` file

`empty.rdp` is zero bytes. `remmina_rdp_file_import_channel()` reads the 2-byte UTF-16 magic, gets
`G_IO_STATUS_EOF` (not an error, so `error` stays NULL), and `plugins/rdp/rdp_file.c:135` dereferences
`error->message`.

Impact: importing an empty file crashes the client.
Fix: treat EOF separately, or print `error ? error->message : "EOF"`.

The harness rejects empty input (`size == 0` returns -1), so **Mayhem will never report this bug**.
libFuzzer always executes the empty input before the corpus, and a crash there fails Mayhem's `-runs=5`
startup probe, which would fail every run. Every non-empty input is delivered unfiltered, including
inputs, including one-byte files.

Reproduce: `/mayhem/fuzz_rdp_file-standalone mayhem/fuzz_rdp_file/known-findings/empty-file-null-error/empty.rdp`
(the harness's own empty-input reject must be bypassed, e.g. by calling the importer directly).
