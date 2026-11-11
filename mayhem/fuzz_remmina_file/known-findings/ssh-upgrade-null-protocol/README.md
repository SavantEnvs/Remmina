# NULL `strcmp` in the pre-1.4 ssh settings upgrade

`repro.remmina` has `ssh_enabled=0` but no `protocol=` key.
`remmina_file_load()` takes the "upgrade ssh params from remmina pre 1.4" branch, where
`remmina_file_get_string(remminafile, "protocol")` returns NULL and `src/remmina_file.c:304` calls
`strcmp(val, "SSH")` on it. UBSan (`nonnull-attribute`) halts; without it this is a NULL read.

Impact: a crafted `.remmina` profile crashes the client when it is loaded.
Fix: guard with `val != NULL &&` before the `strcmp`.

Reproduce: `/mayhem/fuzz_remmina_file-standalone mayhem/fuzz_remmina_file/known-findings/ssh-upgrade-null-protocol/repro.remmina`
