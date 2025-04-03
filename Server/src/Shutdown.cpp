#include "shutdown.h"
#include <iostream>

std::atomic<bool> ShutdownModule::shutdownRequested{false};
std::vector<std::function<void()>> ShutdownModule::cleanupHandlers;

void ShutdownModule::requestShutdown() {
    if (!shutdownRequested.exchange(true)) {
        std::cout << "Initiating shutdown sequence..." << std::endl;
        for (auto& handler : cleanupHandlers) {
            try {
                handler();
            } catch (...) {
                std::cerr << "Error during shutdown handler" << std::endl;
            }
        }
    }
}

bool ShutdownModule::isShutdownRequested() {
    return shutdownRequested.load();
}

void ShutdownModule::registerCleanupHandler(std::function<void()> handler) {
    cleanupHandlers.push_back(handler);
}