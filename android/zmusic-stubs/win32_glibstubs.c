/*
 * android_glibstubs.c - POSIX/pthreads implementation backing win32_glibstubs.h
 * when ZMusic's FluidSynth is built for Android (bionic).
 */
#ifdef __ANDROID__

#define _GNU_SOURCE
#include "win32_glibstubs.h"

#include <errno.h>
#include <sched.h>
#include <sys/stat.h>
#include <unistd.h>

int fluid_g_file_test(const char *path, int flags)
{
    struct stat st;
    if (path == NULL || stat(path, &st) != 0)
        return 0;
    if (flags & G_FILE_TEST_IS_REGULAR)
        return S_ISREG(st.st_mode) ? 1 : 0;
    return 1; /* G_FILE_TEST_EXISTS */
}

int fluid_g_shell_parse_argv(const char *command_line, int *argcp, char ***argvp, void *dummy)
{
    /* Minimal whitespace-splitting implementation of g_shell_parse_argv.
     * FluidSynth only uses it to split soundfont command lines. */
    (void)dummy;
    if (command_line == NULL || argcp == NULL || argvp == NULL)
        return 0;

    enum { MAX_ARGS = 32 };
    char **argv = g_new(char *, MAX_ARGS + 1);
    int argc = 0;
    const char *p = command_line;

    while (*p != '\0' && argc < MAX_ARGS)
    {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
            p++;
        if (*p == '\0')
            break;

        const char *start;
        size_t len;
        if (*p == '"')
        {
            p++;
            start = p;
            while (*p != '\0' && *p != '"')
                p++;
            len = (size_t)(p - start);
            if (*p == '"')
                p++;
        }
        else
        {
            start = p;
            while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
                p++;
            len = (size_t)(p - start);
        }

        argv[argc] = g_new(char, len + 1);
        memcpy(argv[argc], start, len);
        argv[argc][len] = '\0';
        argc++;
    }
    argv[argc] = NULL;

    *argcp = argc;
    *argvp = argv;
    return 1;
}

double fluid_g_get_monotonic_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000000.0 + (double)ts.tv_nsec / 1000.0;
}

void fluid_g_usleep(long usecs)
{
    struct timespec ts;
    ts.tv_sec = usecs / 1000000L;
    ts.tv_nsec = (usecs % 1000000L) * 1000L;
    while (nanosleep(&ts, &ts) != 0 && errno == EINTR)
        ;
}

void fluid_static_rec_mutex_init(pthread_mutex_t *m)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(m, &attr);
    pthread_mutexattr_destroy(&attr);
}

typedef struct
{
    GThreadFunc func;
    void *data;
} fluid_thread_start;

static void *fluid_thread_trampoline(void *arg)
{
    fluid_thread_start *start = (fluid_thread_start *)arg;
    GThreadFunc func = start->func;
    void *data = start->data;
    free(start);
    return func(data);
}

GThread *fluid_g_thread_create(GThreadFunc func, void *data, int joinable, GError **error)
{
    (void)error; /* old thread API never reports errors this way */
    GThread *thread = g_new(GThread, 1);
    if (thread == NULL)
        return NULL;

    /* Detached threads emulate glib's non-joinable threads. */
    fluid_thread_start *start = g_new(fluid_thread_start, 1);
    start->func = func;
    start->data = data;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (!joinable)
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&thread->handle, &attr, fluid_thread_trampoline, start) != 0)
    {
        free(start);
        free(thread);
        pthread_attr_destroy(&attr);
        return NULL;
    }
    pthread_attr_destroy(&attr);
    return thread;
}

void fluid_g_thread_join(GThread *thread)
{
    if (thread == NULL)
        return;
    pthread_join(thread->handle, NULL);
    free(thread);
}

#endif /* __ANDROID__ */
