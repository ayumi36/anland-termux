#ifndef ANW_HIDDEN_H
#define ANW_HIDDEN_H

#include <android/native_window.h>
#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

/* Query constants */
#define ANATIVEWINDOW_QUERY_MIN_UNDEQUEUED_BUFFERS 3

/* --- Minimal vendored gralloc/buffer types -------------------------------
 * Not exposed by the NDK. Layouts copied verbatim from AOSP
 * (frameworks/native libs/nativebase + system/core libcutils) so we can read
 * the dma-buf fd and stride out of a dequeued buffer. Must match the platform
 * ABI exactly. */

#ifndef NATIVE_HANDLE_H_      /* share guard with cutils/native_handle.h */
#define NATIVE_HANDLE_H_
typedef struct native_handle {
    int version;   /* sizeof(native_handle_t) */
    int numFds;    /* number of file-descriptors at &data[0] */
    int numInts;   /* number of ints at &data[numFds] */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"
#endif
    int data[0];   /* numFds + numInts ints; fds come first */
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
} native_handle_t;
#endif

typedef struct android_native_base_t {
    int magic;
    int version;
    void *reserved[4];
    void (*incRef)(struct android_native_base_t *base);
    void (*decRef)(struct android_native_base_t *base);
} android_native_base_t;

typedef struct ANativeWindowBuffer {
    android_native_base_t common;
    int width;
    int height;
    int stride;
    int format;
    int usage_deprecated;
    uintptr_t layerCount;
    void *reserved[1];
    const native_handle_t *handle;   /* handle->data[0] == dma-buf fd */
    uint64_t usage;
    void *reserved_proc[8 - (sizeof(uint64_t) / sizeof(void *))];
} ANativeWindowBuffer;

/* perform() ops / API ids (AOSP system/window.h) */
enum {
    ANW_SET_BUFFER_COUNT = 4,
    ANW_API_CONNECT    = 13,
    ANW_API_DISCONNECT = 14,
    ANW_API_CPU        = 2,
};

/* Full ANativeWindow op table. The NDK type is opaque, but dequeueBuffer needs
 * the window connected to an API first (perform(API_CONNECT)) -- ANativeWindow_lock
 * used to do this internally. perform() has no exported C wrapper, so reach it
 * through the struct. Layout must match the platform ABI exactly. */
struct anw_window {
    android_native_base_t common;
    const uint32_t flags;
    const int   minSwapInterval;
    const int   maxSwapInterval;
    const float xdpi;
    const float ydpi;
    intptr_t    oem[4];
    int (*setSwapInterval)(struct anw_window *, int);
    int (*dequeueBuffer_DEPRECATED)(struct anw_window *, ANativeWindowBuffer **);
    int (*lockBuffer_DEPRECATED)(struct anw_window *, ANativeWindowBuffer *);
    int (*queueBuffer_DEPRECATED)(struct anw_window *, ANativeWindowBuffer *);
    int (*query)(const struct anw_window *, int, int *);
    int (*perform)(struct anw_window *, int, ...);
    int (*cancelBuffer_DEPRECATED)(struct anw_window *, ANativeWindowBuffer *);
    int (*dequeueBuffer)(struct anw_window *, ANativeWindowBuffer **, int *fenceFd);
    int (*queueBuffer)(struct anw_window *, ANativeWindowBuffer *, int fenceFd);
    int (*cancelBuffer)(struct anw_window *, ANativeWindowBuffer *, int fenceFd);
};

/* These layouts are also present in Android 10's nativebase/system/window.h.
 * Validate before dereferencing an OEM operation table. A vendor ABI mismatch
 * must fail cleanly rather than calling an arbitrary address. */
static inline int anw_window_valid(const ANativeWindow *w)
{
    const struct anw_window *aw = (const struct anw_window *)w;
    return aw && aw->common.magic == 0x5f776e64 /* '_wnd' */
        && aw->common.version == sizeof(struct anw_window)
        && aw->perform && aw->query && aw->dequeueBuffer
        && aw->queueBuffer && aw->cancelBuffer;
}

#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(struct anw_window) == 192, "ARM64 native window ABI");
_Static_assert(offsetof(ANativeWindowBuffer, handle) == 96, "ARM64 buffer handle ABI");
_Static_assert(sizeof(ANativeWindowBuffer) == 168, "ARM64 native buffer ABI");
#endif

static inline int anw_api_connect(ANativeWindow *w, int api)
{
    struct anw_window *aw = (struct anw_window *)w;
    return anw_window_valid(w) ? aw->perform(aw, ANW_API_CONNECT, api) : -ENOSYS;
}

static inline int anw_api_disconnect(ANativeWindow *w, int api)
{
    struct anw_window *aw = (struct anw_window *)w;
    return anw_window_valid(w) ? aw->perform(aw, ANW_API_DISCONNECT, api) : -ENOSYS;
}

/* Hidden API function pointers, resolved via dlsym */
typedef int (*pfn_ANativeWindow_setBufferCount)(ANativeWindow *, size_t);
typedef int (*pfn_ANativeWindow_query)(const ANativeWindow *, int, int *);
typedef int (*pfn_ANativeWindow_dequeueBuffer)(ANativeWindow *, ANativeWindowBuffer **, int *fenceFd);
typedef int (*pfn_ANativeWindow_queueBuffer)(ANativeWindow *, ANativeWindowBuffer *, int fenceFd);
typedef int (*pfn_ANativeWindow_cancelBuffer)(ANativeWindow *, ANativeWindowBuffer *, int fenceFd);

struct anw_api {
    pfn_ANativeWindow_setBufferCount setBufferCount;
    pfn_ANativeWindow_query          query;
    pfn_ANativeWindow_dequeueBuffer  dequeueBuffer;
    pfn_ANativeWindow_queueBuffer    queueBuffer;
    pfn_ANativeWindow_cancelBuffer   cancelBuffer;
};

/* The wrappers are VNDK exports, not guaranteed public NDK symbols. When an
 * OEM omits an export, use the same checked operation table that the wrappers
 * call. This keeps dma-buf GPU rendering and fence-fd ownership unchanged. */
static inline int anw_set_buffer_count(ANativeWindow *w, size_t count)
{
    struct anw_window *aw = (struct anw_window *)w;
    return anw_window_valid(w) ? aw->perform(aw, ANW_SET_BUFFER_COUNT, count) : -ENOSYS;
}

static inline int anw_query(const ANativeWindow *w, int what, int *value)
{
    const struct anw_window *aw = (const struct anw_window *)w;
    return anw_window_valid(w) ? aw->query(aw, what, value) : -ENOSYS;
}

static inline int anw_dequeue_buffer(ANativeWindow *w, ANativeWindowBuffer **buffer, int *fence)
{
    struct anw_window *aw = (struct anw_window *)w;
    return anw_window_valid(w) ? aw->dequeueBuffer(aw, buffer, fence) : -ENOSYS;
}

static inline int anw_queue_buffer(ANativeWindow *w, ANativeWindowBuffer *buffer, int fence)
{
    struct anw_window *aw = (struct anw_window *)w;
    return anw_window_valid(w) ? aw->queueBuffer(aw, buffer, fence) : -ENOSYS;
}

static inline int anw_cancel_buffer(ANativeWindow *w, ANativeWindowBuffer *buffer, int fence)
{
    struct anw_window *aw = (struct anw_window *)w;
    return anw_window_valid(w) ? aw->cancelBuffer(aw, buffer, fence) : -ENOSYS;
}

static inline int anw_api_load(struct anw_api *api)
{
    void *lib = dlopen("libnativewindow.so", RTLD_NOW);
    api->setBufferCount = lib ? (pfn_ANativeWindow_setBufferCount) dlsym(lib, "ANativeWindow_setBufferCount") : NULL;
    api->query          = lib ? (pfn_ANativeWindow_query)          dlsym(lib, "ANativeWindow_query") : NULL;
    api->dequeueBuffer  = lib ? (pfn_ANativeWindow_dequeueBuffer)  dlsym(lib, "ANativeWindow_dequeueBuffer") : NULL;
    api->queueBuffer    = lib ? (pfn_ANativeWindow_queueBuffer)    dlsym(lib, "ANativeWindow_queueBuffer") : NULL;
    api->cancelBuffer   = lib ? (pfn_ANativeWindow_cancelBuffer)   dlsym(lib, "ANativeWindow_cancelBuffer") : NULL;

    if (!api->setBufferCount) api->setBufferCount = anw_set_buffer_count;
    if (!api->query)          api->query = anw_query;
    if (!api->dequeueBuffer)  api->dequeueBuffer = anw_dequeue_buffer;
    if (!api->queueBuffer)    api->queueBuffer = anw_queue_buffer;
    if (!api->cancelBuffer)   api->cancelBuffer = anw_cancel_buffer;
    /* Keep lib open for the lifetime of the stored function pointers. */

    return 0;
}

#endif
