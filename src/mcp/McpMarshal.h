// McpMarshal.h: thread marshalling for the Emu80 MCP server.
//
// MCP tool calls arrive on HTTP worker threads, but all emulator state access
// must happen on the main (emulation) thread. Each operation is enqueued and
// the worker blocks until the main thread drains it via ProcessPendingCommands().

#pragma once

#include <functional>
#include <exception>
#include <mutex>
#include <condition_variable>

namespace mcp
{
    struct McpCommand
    {
        std::function<void()>     fn;
        std::exception_ptr        err;
        std::mutex                mtx;
        std::condition_variable   cv;
        bool                      done = false;
    };

    // Call once from the main thread during emulator startup.
    void RegisterMainThread();

    // True after RegisterMainThread() has been called.
    bool IsReady();

    // Submit fn to the main thread and block until it completes. If called from
    // the main thread, fn runs inline to avoid self-deadlock. Returns false if
    // no main thread is registered.
    bool Run(std::function<void()> fn);

    // Execute a marshalled command and signal completion. Called on the main
    // thread from ProcessPendingCommands().
    void Dispatch(McpCommand* cmd);

    // Drain the command queue. Call periodically from the emulation main loop.
    // Non-blocking when the queue is empty.
    void ProcessPendingCommands();
}
