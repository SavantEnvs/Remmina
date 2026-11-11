#!/usr/bin/env bash
#
# mayhem/build.sh — build the Remmina fuzz harnesses and the KAT probe. Runs in the commit image
# (mayhem/Dockerfile) as `mayhem` in /mayhem; see the base image for the build-contract ENV.
#
# Remmina is a GTK application: `remmina` is the only library-ish thing it builds, as the object files
# of the `remmina` executable target. The harnesses link every object of that target except remmina.c.o
# (which holds main()), so the real core (profile loader, public helpers, crypt, plugin manager) is what
# gets fuzzed, instrumented with $SANITIZER_FLAGS. The RDP and SPICE plugins' importers are upstream's own
# rdp_file.c / spice_file.c, #included into the harnesses that fuzz them (their parsers are static).
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

CMAKE_COMMON=(
  -DWITH_CUPS=OFF -DWITH_WWW=OFF -DWITH_PYTHONLIBS=OFF -DWITH_TRANSLATIONS=OFF -DWITH_MANPAGES=OFF
  -DWITH_ICON_CACHE=OFF -DWITH_UPDATE_DESKTOP_DB=OFF -DWITH_NEWS=OFF -DWITH_STATS=OFF -DWITH_TIP=OFF
  -DWITH_FREERDP3=ON -DWITH_SPICE=ON -DWITH_X2GO=ON -DWITH_LIBSSH=ON -DWITH_VTE=ON -DWITH_GCRYPT=ON
)

# 1) Fuzz build: the project instrumented with sanitizers, DWARF 3, and -fsanitize=fuzzer-no-link so the
#    core is coverage-instrumented for libFuzzer. Custom build type: Remmina's Release adds -DNDEBUG.
cmake -S . -B build "${CMAKE_COMMON[@]}" -DCMAKE_BUILD_TYPE=Fuzz \
  -DCMAKE_C_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -O1 -fsanitize=fuzzer-no-link"
cmake --build build --target remmina -j"$MAYHEM_JOBS"

# 2) Oracle build with Remmina's normal flags, kept apart in builddir/ for the KAT probe (step 4).
cmake -S . -B builddir "${CMAKE_COMMON[@]}" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS"
cmake --build builddir --target remmina -j"$MAYHEM_JOBS"

PKGS="gtk+-3.0 gio-unix-2.0 gmodule-2.0 libssh libgcrypt avahi-client avahi-ui-gtk3 openssl vte-2.91 libsodium json-glib-1.0 libcurl wayland-client"
for m in ayatana-appindicator3-0.1 appindicator3-0.1; do
  if pkg-config --exists "$m"; then PKGS="$PKGS $m"; break; fi
done
# shellcheck disable=SC2086
LIBS="$(pkg-config --libs $PKGS)"
# shellcheck disable=SC2086
INCS="$(pkg-config --cflags $PKGS freerdp-client3 winpr3 spice-client-gtk-3.0)"
DEFS="-DHAVE_LIBSSH -DHAVE_LIBGCRYPT -DHAVE_LIBVTE -DDISABLE_NEWS -DDISABLE_STATS -DDISABLE_TIP"
PROJ_INCS="-I$SRC -I$SRC/src -I$SRC/src/include -I$SRC/plugins -I$SRC/plugins/rdp -I$SRC/plugins/spice -I$SRC/mayhem"

core_objs() { find "$SRC/$1/src/CMakeFiles/remmina.dir" -name '*.c.o' ! -name remmina.c.o | sort; }
FUZZ_OBJS=$(core_objs build)
ORACLE_OBJS=$(core_objs builddir)

# 3) Harnesses: libFuzzer binary + non-fuzzer -standalone reproducer, each with lsan_off.
OBJ=/tmp/mayhem-obj
rm -rf "$OBJ"; mkdir -p "$OBJ"
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o "$OBJ/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OBJ/standalone_main.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -O1 -fsanitize=fuzzer-no-link -c mayhem/remmina_globals.c -o "$OBJ/globals.o"
$CC -O1 $COVERAGE_FLAGS -c mayhem/remmina_globals.c -o "$OBJ/globals_oracle.o"

HCFLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -O1 -fsanitize=fuzzer-no-link $DEFS $PROJ_INCS -Ibuild/generated $INCS"

build_harness() {
  local name="$1"; shift
  # shellcheck disable=SC2086
  $CC $HCFLAGS -c "mayhem/$name.c" -o "$OBJ/$name.o"
  # shellcheck disable=SC2086
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$OBJ/$name.o" "$@" "$OBJ/lsan_off.o" "$OBJ/globals.o" $FUZZ_OBJS $LIBS -o "$SRC/$name"
  # shellcheck disable=SC2086
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS "$OBJ/standalone_main.o" "$OBJ/$name.o" "$@" "$OBJ/lsan_off.o" "$OBJ/globals.o" $FUZZ_OBJS $LIBS -o "$SRC/$name-standalone"
}
build_harness fuzz_remmina_file
build_harness fuzz_public_utils
build_harness fuzz_rdp_file
build_harness fuzz_spice_file

# 4) KAT probe: normal flags, linked against the oracle build's objects; mayhem/test.sh only runs it.
# shellcheck disable=SC2086
$CC -O1 $COVERAGE_FLAGS $DEFS $PROJ_INCS -Ibuilddir/generated $INCS mayhem/kat_probe.c "$OBJ/globals_oracle.o" $ORACLE_OBJS $LIBS \
  -o "$SRC/builddir/kat_probe"
