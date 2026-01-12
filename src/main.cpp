#include <iostream>
#include <string>
#include "tui.h"

#ifdef WITH_GUI
#include "gui.h"
#include <QtGlobal>
#endif

int main(int argc, char *argv[]) {
    bool runGui = false;
#ifdef WITH_GUI
    if (argc > 1 && std::string(argv[1]) == "gui") {
        runGui = true;
    }
#endif

    if (runGui) {
#ifdef WITH_GUI
        Q_INIT_RESOURCE(icons);
        return run_gui(argc, argv);
#else
        std::cerr << "ERROR: GUI not available. Please compile with GUI support." << std::endl;
        return 1;
#endif
    } else {
        try{
            return run_tui_safe();
        } catch (const std::runtime_error &e) {
            std::cerr << "ERROR: " << e.what() << "\n";
            return 1;
        }
    }
}
