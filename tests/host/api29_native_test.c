#define _GNU_SOURCE
#include <assert.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

static int have_library;
static int lookup_count;
static void *test_dlopen(const char *name, int flags) {
    (void)name; (void)flags;
    return have_library ? &have_library : NULL;
}
static void *test_dlsym(void *lib, const char *name) {
    assert(lib); (void)name;
    lookup_count++;
    return NULL; // Exercise missing VNDK wrappers, even with libnativewindow.
}
#define dlopen test_dlopen
#define dlsym test_dlsym
#include "../../app/src/main/jni/anw_hidden.h"
#undef dlopen
#undef dlsym

// Compile the actual Android shared-memory branch. Deliberately leave these
// forbidden symbols undefined: a regression to either call fails the link.
#define memfd_create forbidden_api30_memfd_create
#define ftruncate forbidden_ashmem_ftruncate
#include "../../app/src/main/jni/anland_core/libdisplay_consumer/display_consumer.c"
#undef memfd_create
#undef ftruncate

static int fail_shared_memory;
int ASharedMemory_create(const char *name, size_t size) {
    // A regular unlinked mmap-able fd stands in for the Android allocator.
    // This tests our fd protocol, not Android's ashmem implementation.
    assert(strcmp(name, "buf_select") == 0);
    assert(size == sizeof(uint32_t));
    if (fail_shared_memory) { errno = ENOMEM; return -1; }
    char path[] = "/tmp/anland-shm-test-XXXXXX";
    int fd = mkstemp(path);
    assert(fd >= 0);
    assert(unlink(path) == 0);
    assert(ftruncate(fd, (off_t)size) == 0);
    return fd; // Let the real create_shm code set FD_CLOEXEC.
}

static int last_operation, last_fence;
static size_t last_count;
static ANativeWindowBuffer test_buffer;
static int window_perform(struct anw_window *w, int operation, ...) {
    assert(w);
    last_operation = operation;
    va_list args;
    va_start(args, operation);
    if (operation == ANW_SET_BUFFER_COUNT)
        last_count = va_arg(args, size_t);
    else
        assert(va_arg(args, int) == ANW_API_CPU);
    va_end(args);
    return 0;
}
static int window_query(const struct anw_window *w, int what, int *value) {
    assert(w && what == ANATIVEWINDOW_QUERY_MIN_UNDEQUEUED_BUFFERS);
    *value = 2;
    return 0;
}
static int window_dequeue(struct anw_window *w, ANativeWindowBuffer **buffer, int *fence) {
    assert(w);
    *buffer = &test_buffer; *fence = 41;
    return 0;
}
static int window_queue(struct anw_window *w, ANativeWindowBuffer *buffer, int fence) {
    assert(w && buffer == &test_buffer);
    last_fence = fence;
    return 0;
}

static void test_window_fallbacks(void) {
    struct anw_window window = {
        .common = {.magic = 0x5f776e64, .version = sizeof(struct anw_window)},
        .perform = window_perform, .query = window_query,
        .dequeueBuffer = window_dequeue, .queueBuffer = window_queue,
        .cancelBuffer = window_queue,
    };
    ANativeWindow *w = (ANativeWindow *)&window;
    for (have_library = 0; have_library <= 1; have_library++) {
        struct anw_api api;
        assert(anw_api_load(&api) == 0);
        assert(anw_api_connect(w, ANW_API_CPU) == 0);
        assert(last_operation == ANW_API_CONNECT);
        assert(api.setBufferCount(w, (size_t)4) == 0 && last_count == 4);
        int value;
        assert(api.query(w, ANATIVEWINDOW_QUERY_MIN_UNDEQUEUED_BUFFERS, &value) == 0);
        assert(value == 2);
        ANativeWindowBuffer *buffer;
        int fence;
        assert(api.dequeueBuffer(w, &buffer, &fence) == 0);
        assert(fence == 41 && buffer == &test_buffer);
        assert(api.queueBuffer(w, buffer, 42) == 0 && last_fence == 42);
        assert(api.cancelBuffer(w, buffer, 43) == 0 && last_fence == 43);
        assert(anw_api_disconnect(w, ANW_API_CPU) == 0);
        assert(last_operation == ANW_API_DISCONNECT);
        window.common.version = 1;
        assert(api.setBufferCount(w, 4) == -ENOSYS);
        assert(anw_api_connect(w, ANW_API_CPU) == -ENOSYS);
        window.common.version = sizeof(struct anw_window);
    }
    assert(lookup_count == 5);
    window.common.magic = 0;
    assert(!anw_window_valid(w));
    assert(!anw_window_valid(NULL));
}

static void test_shared_memory_protocol(void) {
    display_ctx ctx = {.shm_fd = -1};
    assert(create_shm(&ctx) == 0);
    assert(fcntl(ctx.shm_fd, F_GETFD) & FD_CLOEXEC);
    assert(*ctx.shm_ptr == 0);
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    char sent = 'S', received = 0;
    int peer_fd = -1, count = 0;
    assert(send_fds(sockets[0], &sent, 1, &ctx.shm_fd, 1) == 0);
    assert(recv_fds(sockets[1], &received, 1, &peer_fd, 1, &count) == 1);
    assert(count == 1 && received == sent);
    volatile uint32_t *peer = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE,
                                   MAP_SHARED, peer_fd, 0);
    assert(peer != MAP_FAILED);
    *ctx.shm_ptr = 7;
    assert(*peer == 7);
    *peer = 3;
    assert(*ctx.shm_ptr == 3);
    munmap((void *)peer, sizeof(uint32_t)); close(peer_fd);
    munmap((void *)ctx.shm_ptr, sizeof(uint32_t)); close(ctx.shm_fd);
    close(sockets[0]); close(sockets[1]);
    fail_shared_memory = 1;
    ctx.shm_fd = -1; ctx.shm_ptr = NULL;
    assert(create_shm(&ctx) == -1 && ctx.shm_fd == -1 && ctx.shm_ptr == NULL);
}

int main(void) {
    test_window_fallbacks();
    test_shared_memory_protocol();
    puts("PASS: native-window ABI rejection, missing-export fallbacks, fence forwarding,");
    puts("      Android-branch shared-memory fd transfer, two-way mmap, allocator failure.");
    puts("Host checks only; Android compilation and device execution are still required.");
    return 0;
}
