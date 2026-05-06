#include <iostream>
#include "cli.h"

int main(int argc, char *argv[]) {
    try{
        return run_cli_safe();
    } catch (const std::runtime_error &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
