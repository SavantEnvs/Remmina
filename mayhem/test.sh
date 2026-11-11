#!/usr/bin/env bash
#
# mayhem/test.sh — run the Remmina known-answer probe (built by mayhem/build.sh against the normally-built
# core) and compare every line it prints against the expected value. Builds nothing.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

PROBE="$SRC/builddir/kat_probe"
if [ ! -x "$PROBE" ]; then
  echo "missing $PROBE — mayhem/build.sh should have built it" >&2
  emit_ctrf "remmina-kat" 0 1
  exit $?
fi

# Expected known answers: id=value (the encrypt value is 3DES-CBC under the fixed test key in remmina_fuzz_init.h).
EXPECTED='server_port.host_port=rdp.example.com|3390
server_port.default=example.org|3389
server_port.ipv6=::1|5901
resolution.ok=1|1920|1080
resolution.bad=0
str_replace=a+b+c
replace_all=bazz bar bazz|2
strip=ab
string_array.find=1
string_array.after_remove=apple,fig
crypt.encrypt=PKLteKHkPDI=
crypt.roundtrip=hunter2
file.name=Test
file.server=10.0.0.5:3390
file.width=1280
file.height=800
file.password=hunter2'

OUT="$(timeout 120 "$PROBE" </dev/null 2>/dev/null)"
rc=$?
echo "$OUT"
[ "$rc" -eq 0 ] || echo "probe exited with $rc" >&2

passed=0 failed=0
while IFS= read -r exp; do
  if printf '%s\n' "$OUT" | grep -qxF "KAT $exp"; then
    passed=$((passed + 1))
  else
    echo "FAIL: expected 'KAT $exp'" >&2
    failed=$((failed + 1))
  fi
done <<< "$EXPECTED"
[ "$rc" -eq 0 ] || failed=$((failed + 1))

emit_ctrf "remmina-kat" "$passed" "$failed"
