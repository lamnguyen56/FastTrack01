/**
 * @file main.cpp
 * @brief Daemon process entry point.
 */

#include "CanApplication.h"
/**
 * @brief Start the daemon through its application composition module.
 * @return Application exit status consumed by systemd restart handling.
 */
int main() {
    return canbus::CanApplication{}.run();
}
