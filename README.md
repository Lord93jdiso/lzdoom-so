# LZDoom for Android (.so)

LZDoom (GZDoom legacy fork, branch `l4.14dev` from [drfrag666/lzdoom](https://github.com/drfrag666/lzdoom))
built as a **shared library** (`liblzdoom.so`) for embedding into an existing
Android application. GitHub Actions builds it automatically on every push.

## Artifacts

Every CI run produces:

| Artifact | Contents |
|---|---|
| `lzdoom-android-arm64-v8a` | `arm64-v8a/liblzdoom.so` + `*.pk3` game resources |
| `lzdoom-android-armeabi-v7a` | `armeabi-v7a/liblzdoom.so` + `*.pk3` game resources |
| `lzdoom-android-all` | everything above in one zip |

**Important:** the `.pk3` files (`lzdoom.pk3`, `lzdoom_bm.pk3`, …) are *required*
at runtime — put them next to your IWADs and pass their path via `-file`/iwad
lookup paths (or place them in `files_dir`).

## Build configuration

- Renderer: **OpenGL ES 2** backend (`HAVE_GLES2=ON`), Vulkan disabled
- Static SDL2 + static ZMusic linked inside the `.so` — no extra `.so` needed
- Discord RPC, GTK, JIT, libvpx video: disabled / stubbed
- API level 24 (Android 7.0+)
- STL: `c++_static`

## C API

See [`android/lzdoom_api.h`](android/lzdoom_api.h):

```c
int  lzdoom_main(int argc, const char** argv, const char* files_dir);
void lzdoom_request_exit(void);
void lzdoom_push_key(int down, int scancode, int unicode);
void lzdoom_push_text(const char* text);
void lzdoom_push_touch(int action, int finger_id, float x, float y);
```

### Example JNI usage

```kotlin
// Kotlin side
System.loadLibrary("lzdoom")

external fun lzdoomMain(argc: Int, argv: Array<String>, filesDir: String): Int
```

```cpp
// Native side (your app's jni)
#include "lzdoom_api.h"

extern "C" JNIEXPORT jint JNICALL
Java_com_example_yourapp_NativeLib_lzdoomMain(JNIEnv* env, jclass,
                                              jint argc, jobjectArray argvArr,
                                              jstring filesDir) {
    const char* dir = env->GetStringUTFChars(filesDir, nullptr);
    std::vector<const char*> args;
    args.push_back("lzdoom");
    // append "-iwad /path/doom2.wad -file /path/mod.pk3" etc.
    int rc = lzdoom_main(argc, args.data(), dir);   // blocks until exit
    env->ReleaseStringUTFChars(filesDir, dir);
    return rc;
}
```

Call `lzdoom_main` from a dedicated game thread with a live SDL2 surface
(e.g. under `SDLActivity`). The engine renders through SDL2 and reads input
from the SDL event queue; `lzdoom_push_*` helpers inject synthetic events if
you don't use SDLActivity's input.

### Launch arguments

Same as the desktop executable:

```
-iwad /path/doom2.wad          # IWAD (doom.wad, doom2.wad, ...)
-file /path/mod.pk3            # load mod/PWAD (repeatable)
+map E1M1                      # console commands
-warp 1                        # warp to level
```

## Local build

Requires: CMake ≥ 3.16, Ninja, Android NDK r27.

```bash
# 1) native tools
cmake -S . -B native-build -DCMAKE_BUILD_TYPE=Release -DHAVE_VULKAN=OFF
cmake --build native-build --target zipdir

# 2) build static SDL2 + ZMusic for your ABI (see .github/workflows/android.yml)

# 3) engine
SDL2DIR=<sdl-stage> cmake -S . -B build -GNinja \
  -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 \
  -DANDROID_STL=c++_static -DFORCE_CROSSCOMPILE=ON \
  -DIMPORT_EXECUTABLES=$PWD/native-build/ImportExecutables.cmake \
  -DHAVE_VULKAN=OFF -DNO_GTK=ON -DDYN_OPENAL=ON \
  -DZMUSIC_INCLUDE_DIR=<zmusic-stage> -DZMUSIC_LIBRARIES=<zmusic-stage>/lib/libzmusic.a
cmake --build build
```

## License & credits

Upstream code: GPL-3.0 — see [LICENSE](LICENSE) and upstream
[drfrag666/lzdoom](https://github.com/drfrag666/lzdoom).
Android library-mode changes are minimal patches on top of upstream.
