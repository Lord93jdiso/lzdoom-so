/*
** lzdoom_api.h - C API for embedding LZDoom in an Android app.
**
** Usage: call lzdoom_main() from a dedicated game thread. It blocks until
** the engine exits (user quits or lzdoom_request_exit()). The engine renders
** through SDL2, so run it under SDLActivity / your existing SDL integration.
*/

#ifndef LZDOOM_API_H
#define LZDOOM_API_H

#ifdef __cplusplus
extern "C" {
#endif

/*
** Runs the engine. Blocks until shutdown. Returns engine exit code.
**
** argc/argv: command line as you would pass to the lzdoom executable,
**   e.g.: {"lzdoom", "-iwad", "/path/doom2.wad", "-file", "/path/mod.pk3"}
**
** files_dir: writable app directory used as HOME and working directory
**   (saves/config/lzdoom.ini live here).
*/
int  lzdoom_main(int argc, const char** argv, const char* files_dir);

/*
 * Sets the writable game dir BEFORE launching via SDLActivity/SDL_main.
 * Also sets HOME/chdir there.
 */
void lzdoom_set_files_dir(const char* files_dir);

/* Asks the engine to shut down (thread-safe). */
void lzdoom_request_exit(void);

/* Keyboard input. scancode = SDL_Scancode, unicode = SDL_Keycode. */
void lzdoom_push_key(int down, int scancode, int unicode);

/* Text input (for console/chat). */
void lzdoom_push_text(const char* text);

/* Touch input. action: 0 = down, 1 = up, other = motion. */
void lzdoom_push_touch(int action, int finger_id, float x, float y);

#ifdef __cplusplus
}
#endif

#endif // LZDOOM_API_H
