#include "gui.h"
#include <QtGlobal>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

int main(int argc, char *argv[]) {
#ifdef _WIN32
    HANDLE hMutex = CreateMutexA(NULL, FALSE, "mkpass_gui_mutex");
#endif
    Q_INIT_RESOURCE(icons);
    int ret = run_gui(argc, argv);
#ifdef _WIN32
    if (hMutex != NULL) {
        CloseHandle(hMutex);
    }
#endif
    return ret;
}
