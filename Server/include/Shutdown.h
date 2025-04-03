#ifndef SHUTDOWN_MODULE_H
#define SHUTDOWN_MODULE_H

#include <atomic>
#include <functional>
#include <vector>

class ShutdownModule {
public:
    static void requestShutdown();
    static bool isShutdownRequested();

    // Register cleanup callbacks
    static void registerCleanupHandler(std::function<void()> handler);

private:
    static std::atomic<bool> shutdownRequested;
    static std::vector<std::function<void()>> cleanupHandlers;
};

#endif // SHUTDOWN_MODULE_H