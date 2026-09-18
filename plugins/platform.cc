#include "platform.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <cstdint>
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

using namespace plugins;

bool platform::isTerminal(std::FILE *stream) {
#ifdef _WIN32
    return _isatty(_fileno(stream)) != 0;
#else
    return isatty(fileno(stream)) != 0;
#endif
}

unsigned platform::terminalWidth(std::FILE *stream) {
#ifdef _WIN32
    const intptr_t osf = _get_osfhandle(_fileno(stream));
    if (osf == -1)
        return 0;
    const HANDLE handle = reinterpret_cast<HANDLE>(osf);
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(handle, &info))
        return 0;
    const int width = info.srWindow.Right - info.srWindow.Left + 1;
    return width > 0 ? static_cast<unsigned>(width) : 0;
#else
    winsize ws{};
    if (ioctl(fileno(stream), TIOCGWINSZ, &ws) != 0)
        return 0;
    return static_cast<unsigned>(ws.ws_col);
#endif
}

void platform::enableUtf8Console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
}
