/* The command-line switches that src/remmina.c (main) defines at file scope and the rest of the core reads.
 * Harnesses link every core object except remmina.c.o, so they provide the (all-off) defaults. */
typedef int gboolean; /* glib's gboolean, without needing its include path */

gboolean kioskmode;
gboolean imode;
gboolean disablenews;
gboolean disablestats;
gboolean disabletoolbar;
gboolean fullscreen;
gboolean extrahardening;
gboolean disabletrayicon;
