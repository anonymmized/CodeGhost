#pragma once

#ifdef __linux__
#include <sys/inotify.h>
#else

#include <cerrno>
#include <cstdint>

struct inotify_event {
    int wd;
    uint32_t mask;
    uint32_t cookie;
    uint32_t len;
    char name[0];
};

inline constexpr uint32_t IN_ACCESS = 0x00000001;
inline constexpr uint32_t IN_MODIFY = 0x00000002;
inline constexpr uint32_t IN_ATTRIB = 0x00000004;
inline constexpr uint32_t IN_CLOSE_WRITE = 0x00000008;
inline constexpr uint32_t IN_CLOSE_NOWRITE = 0x00000010;
inline constexpr uint32_t IN_OPEN = 0x00000020;
inline constexpr uint32_t IN_MOVED_FROM = 0x00000040;
inline constexpr uint32_t IN_MOVED_TO = 0x00000080;
inline constexpr uint32_t IN_CREATE = 0x00000100;
inline constexpr uint32_t IN_DELETE = 0x00000200;
inline constexpr uint32_t IN_DELETE_SELF = 0x00000400;
inline constexpr uint32_t IN_MOVE_SELF = 0x00000800;
inline constexpr uint32_t IN_UNMOUNT = 0x00002000;
inline constexpr uint32_t IN_Q_OVERFLOW = 0x00004000;
inline constexpr uint32_t IN_IGNORED = 0x00008000;
inline constexpr uint32_t IN_ISDIR = 0x40000000;

inline int inotify_init() {
    errno = ENOSYS;
    return -1;
}

inline int inotify_add_watch(int, const char*, uint32_t) {
    errno = ENOSYS;
    return -1;
}

inline int inotify_rm_watch(int, int) {
    errno = ENOSYS;
    return -1;
}

#endif
