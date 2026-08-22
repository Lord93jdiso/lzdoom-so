#!/usr/bin/env python3
"""Patches ZMusic sources so FluidSynth builds for Android:
- replaces the Windows-only glib stubs with POSIX/pthread versions
- routes the ANDROID build through the WITH_GLIB_STUBS code path instead of
  requiring host glib via pkg-config.
"""
import os
import shutil
import sys

zmusic = sys.argv[1]
stubs = os.path.join(os.path.dirname(os.path.abspath(__file__)), "zmusic-stubs")

utils_dir = os.path.join(zmusic, "thirdparty", "fluidsynth", "src", "utils")
for name in ("win32_glibstubs.h", "win32_glibstubs.c"):
    shutil.copy(os.path.join(stubs, name), utils_dir)

cm_path = os.path.join(zmusic, "thirdparty", "fluidsynth", "src", "CMakeLists.txt")
with open(cm_path) as f:
    text = f.read()

old_sources_block = "if ( WIN32 )\n    set( fluidsynth_SOURCES"
new_sources_block = "if ( WIN32 OR ANDROID )\n    set( fluidsynth_SOURCES"
assert old_sources_block in text, "fluidsynth sources block not found"
text = text.replace(old_sources_block, new_sources_block)

old_glib_block = "if (NOT WIN32)\n   find_package(PkgConfig REQUIRED)"
new_glib_block = "if (NOT WIN32 AND NOT ANDROID)\n   find_package(PkgConfig REQUIRED)"
assert old_glib_block in text, "glib pkg-config block not found"
text = text.replace(old_glib_block, new_glib_block)

# fluid_sys.h: skip <glib/gstdio.h> on Android and pull in the stub header instead.
sys_h_path = os.path.join(utils_dir, "fluid_sys.h")
with open(sys_h_path) as f:
    sys_h = f.read()

old_gstdio = "#ifndef WIN32\n#include <glib/gstdio.h>\n#endif"
new_gstdio = """#if !defined(WIN32) && !defined(__ANDROID__)
#include <glib/gstdio.h>
#elif defined(__ANDROID__) && defined(WITH_GLIB_STUBS)
#include "win32_glibstubs.h"
#endif"""
assert old_gstdio in sys_h, "fluid_sys.h gstdio include not found"
sys_h = sys_h.replace(old_gstdio, new_gstdio)

with open(sys_h_path, "w") as f:
    f.write(sys_h)

if "WITH_GLIB_STUBS" not in text:
    text += (
        "\nif(ANDROID)\n"
        "\t# No glib on Android: use bundled pthread-based glib stubs.\n"
        "\ttarget_compile_definitions(fluidsynth PRIVATE WITH_GLIB_STUBS)\n"
        "endif()\n"
    )

with open(cm_path, "w") as f:
    f.write(text)

# config.h bug: HAVE_WINDOWS_H is defined unconditionally, which drags the
# Windows-only GStatBuf fallback into Android builds (bionic also macros
# st_mtime). Guard it properly.
config_h_path = os.path.join(zmusic, "thirdparty", "fluidsynth", "src", "config.h")
with open(config_h_path) as f:
    config_h = f.read()

old_windows = """/* Define to 1 if you have the <windows.h> header file. */
#define HAVE_WINDOWS_H 1"""
new_windows = """/* Define to 1 if you have the <windows.h> header file. */
#ifdef _WIN32
#define HAVE_WINDOWS_H 1
#endif"""
assert old_windows in config_h, "HAVE_WINDOWS_H block not found in config.h"
config_h = config_h.replace(old_windows, new_windows)

with open(config_h_path, "w") as f:
    f.write(config_h)

print("ZMusic patched for Android")
