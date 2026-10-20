#include "Logger.h"
#include <thread>
#include <vector>

int main() {
    int suppressed = 0;
    logging::setLevel(logging::Level::Info);
    LOG_D("hidden ", ++suppressed);
    if (suppressed != 0) { return 1; }
    LOG_I("info message");
    LOG_W("warning message");
    LOG_E("error message");
    logging::setLevel(logging::Level::Off);
    LOG_E("hidden off");
    logging::setLevel(logging::Level::Debug);
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([i]() { LOG_D("worker=", i); });
    }
    for (std::thread& worker : workers) { worker.join(); }
    return 0;
}
