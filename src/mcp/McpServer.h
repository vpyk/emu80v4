// McpServer.h: embedded MCP (Model Context Protocol) server for Emu80.
//
// Runs an HTTP listener bound to 127.0.0.1 on a background thread for the whole
// lifetime of the application. AI clients connect to the already-running
// emulator and call tools (registers, memory, breakpoints, ...). Every tool
// runs its emulator access on the main thread via mcp::Run (see McpMarshal.h).
//
// Uses PIMPL to keep httplib / nlohmann-json out of public headers.

#pragma once

#include <memory>

class CMcpServer
{
    public:
        CMcpServer();
        ~CMcpServer();

        bool            Start(unsigned short port);
        void            Stop();
        bool            IsRunning() const;
        unsigned short  GetPort() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_pImpl;
};

extern CMcpServer g_McpServer;
