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

if "WITH_GLIB_STUBS" not in text:
    text += (
        "\nif(ANDROID)\n"
        "\t# No glib on Android: use bundled pthread-based glib stubs.\n"
        "\ttarget_compile_definitions(fluidsynth PRIVATE WITH_GLIB_STUBS)\n"
        "endif()\n"
    )

with open(cm_path, "w") as f:
    f.write(text)

print("ZMusic patched for Android")
