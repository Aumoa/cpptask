// Copyright 2020-2025 Aumoa.lib. All right reserved.

#include "cpptask/task.h"
#include <csignal>
#include <stop_token>
#include <print>
#include <functional>
#include <chrono>

std::stop_source g_ss;

void sigint_handler(int)
{ 
    g_ss.request_stop();
}

cpptask::task<int> main_async(std::stop_token s_token)
{
    std::println("main_async: tid: {}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
    co_await cpptask::task<>::delay(std::chrono::milliseconds(500), s_token);
    std::println("main_async after delay: tid: {}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
    co_await cpptask::task<>::run([s_token]()
    {
        std::println("task body: tid: {}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
        for (int i = 0; i < 10; i++)
        {
            if (s_token.stop_requested())
            {
                std::println("task body detected stop request: tid: {}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
                throw cpptask::task_canceled_exception();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::println("task body working... {}", i);
        }
    }, s_token);
    co_return 0;
}

int main()
{
    signal(SIGINT, sigint_handler);

    try
    {
        std::println("main: tid: {}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
        int e = main_async(g_ss.get_token()).get_result();
        std::println("e: {}, tid: {}", e, std::hash<std::thread::id>{}(std::this_thread::get_id()));
    }
    catch (const cpptask::task_canceled_exception&)
    {
        std::println("task canceled.");
        return -1;
    }
}