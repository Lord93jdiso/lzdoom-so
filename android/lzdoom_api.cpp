/*
** lzdoom_api.cpp
** Library-mode entry points for embedding LZDoom into an Android app.
** Replaces the main() function from i_main.cpp (which is compiled out
** when __ANDROID__ is defined).
*/

#ifdef __ANDROID__

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

#include "cmdlib.h"
#include "m_argv.h"

int GameMain();
void I_StartupJoysticks();

static std::vector<char*> g_argStorage;

extern "C"
{

// Files dir remembered for the SDL_main entry point (set by the host app
// before SDL starts the engine thread).
static std::string g_filesDir;

int lzdoom_main(int argc, const char** argv, const char* files_dir);

__attribute__((visibility("default")))
void lzdoom_set_files_dir(const char* dir)
{
	g_filesDir = (dir != nullptr) ? dir : "";
	setenv("HOME", g_filesDir.c_str(), 1);
	chdir(g_filesDir.c_str());
}

/* Entry point for stock SDLActivity: dlsym(SDL_main) finds this. */
__attribute__((visibility("default")))
int SDL_main(int argc, char** argv)
{
	return lzdoom_main(argc, argv, g_filesDir.c_str());
}

__attribute__((visibility("default")))
__attribute__((visibility("default")))
int lzdoom_main(int argc, const char** argv, const char* files_dir)
{
	setenv("LC_NUMERIC", "C", 1);
	setlocale(LC_ALL, "C");

	if (SDL_Init(0) < 0)
	{
		fprintf(stderr, "Could not initialize SDL:\n%s\n", SDL_GetError());
		return -1;
	}

	if (files_dir != nullptr)
	{
		setenv("HOME", files_dir, 1);
		chdir(files_dir);
	}

	// Build mutable argv storage and keep it alive for the process lifetime.
	g_argStorage.clear();
	g_argStorage.push_back(const_cast<char*>("lzdoom"));
	for (int i = 1; i < argc; ++i)
	{
		g_argStorage.push_back(const_cast<char*>(argv[i]));
	}
	g_argStorage.push_back(nullptr);

	int realArgc = (int)g_argStorage.size() - 1;
	char** realArgv = g_argStorage.data();

	Args = new FArgs(realArgc, realArgv);

	if (files_dir != nullptr)
	{
		progdir = files_dir;
		if (progdir.Back() != '/')
			progdir += "/";
	}

	I_StartupJoysticks();

	const int result = GameMain();

	SDL_Quit();
	return result;
}

__attribute__((visibility("default")))
void lzdoom_request_exit(void)
{
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

__attribute__((visibility("default")))
void lzdoom_push_key(int down, int scancode, int unicode)
{
	SDL_Event event;
	event.type = down ? SDL_KEYDOWN : SDL_KEYUP;
	event.key.keysym.scancode = (SDL_Scancode)scancode;
	event.key.keysym.sym = (SDL_Keycode)unicode;
	event.key.repeat = 0;
	SDL_PushEvent(&event);
}

__attribute__((visibility("default")))
void lzdoom_push_text(const char* text)
{
	if (text == nullptr) return;
	SDL_Event event;
	event.type = SDL_TEXTINPUT;
	strncpy(event.text.text, text, sizeof(event.text.text) - 1);
	event.text.text[sizeof(event.text.text) - 1] = '\0';
	SDL_PushEvent(&event);
}

__attribute__((visibility("default")))
void lzdoom_push_touch(int action, int finger_id, float x, float y)
{
	SDL_Event event;
	switch (action)
	{
	case 0: event.type = SDL_FINGERDOWN; break;
	case 1: event.type = SDL_FINGERUP;   break;
	default: event.type = SDL_FINGERMOTION; break;
	}
	event.tfinger.fingerId = (SDL_FingerID)finger_id;
	event.tfinger.x = x;
	event.tfinger.y = y;
	event.tfinger.dx = 0.0f;
	event.tfinger.dy = 0.0f;
	event.tfinger.pressure = 1.0f;
	SDL_PushEvent(&event);
}

} // extern "C"

#endif // __ANDROID__
