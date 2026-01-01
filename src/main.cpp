#include <iostream>
#include "tui.h"


int main() {
    try{
        return run_tui_safe();
    } catch (const std::runtime_error &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
