// Copyright 2020-2025 Aumoa.lib. All right reserved.

#include "cpptask/task.h"
#include <csignal>
#include <stop_token>
#include <print>
#include <functional>

std::stop_source g_ss;

void sigint_handler(int)
{ 
    g_ss.request_stop();
}

cpptask::task<int> main_async()
{
    std::println("main_async: tid: {}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
    co_return 0;
}

int main()
{
    signal(SIGINT, sigint_handler);

    try
    {
        std::println("main: tid: {}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
        int e = main_async().get_result();
        std::println("e: {}, tid: {}", e, std::hash<std::thread::id>{}(std::this_thread::get_id()));
    }
    catch (const cpptask::task_canceled_exception&)
    {
        std::println("task canceled.");
        return -1;
    }
}