#include "gui.h"
#include <QtGlobal>

int main(int argc, char *argv[]) {
    Q_INIT_RESOURCE(icons);
    return run_gui(argc, argv);
}
