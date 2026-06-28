// McpMarshal.cpp: C++17 thread marshalling implementation.

#include "McpMarshal.h"

#include <deque>
#include <thread>

namespace mcp
{
    namespace
    {
        std::thread::id            g_mainThreadId;
        bool                       g_ready = false;
        std::mutex                 g_queueMtx;
        std::deque<McpCommand*>    g_queue;
    }

    void RegisterMainThread()
    {
        g_mainThreadId = std::this_thread::get_id();
        g_ready = true;
    }

    bool IsReady()
    {
        return g_ready;
    }

    void Dispatch(McpCommand* cmd)
    {
        if (!cmd)
            return;

        try {
            if (cmd->fn)
                cmd->fn();
        } catch (...) {
            cmd->err = std::current_exception();
        }

        {
            std::lock_guard<std::mutex> lk(cmd->mtx);
            cmd->done = true;
        }
        cmd->cv.notify_one();
    }

    bool Run(std::function<void()> fn)
    {
        if (!g_ready)
            return false;

        // Already on the main thread: run inline to avoid deadlock.
        if (std::this_thread::get_id() == g_mainThreadId) {
            if (fn)
                fn();
            return true;
        }

        McpCommand cmd;
        cmd.fn = std::move(fn);

        {
            std::lock_guard<std::mutex> lk(g_queueMtx);
            g_queue.push_back(&cmd);
        }

        // Block until the main thread runs the command. The wait is unbounded
        // because `cmd` lives on this stack and `fn`'s by-reference captures
        // must stay valid until execution completes.
        {
            std::unique_lock<std::mutex> lk(cmd.mtx);
            cmd.cv.wait(lk, [&cmd] { return cmd.done; });
        }

        if (cmd.err)
            std::rethrow_exception(cmd.err);

        return true;
    }

    void ProcessPendingCommands()
    {
        if (!g_ready)
            return;

        McpCommand* cmd = nullptr;

        for (;;) {
            {
                std::lock_guard<std::mutex> lk(g_queueMtx);
                if (g_queue.empty())
                    return;
                cmd = g_queue.front();
                g_queue.pop_front();
            }

            Dispatch(cmd);
        }
    }
}
