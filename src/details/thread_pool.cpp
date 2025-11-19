// Copyright 2020-2025 Aumoa. All right reserved.

#include "cpptask/details/thread_pool.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>

#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <processthreadsapi.h>
#elif defined(PLATFORM_LINUX)
    #include <pthread.h>
    #include <unistd.h>
#elif defined(PLATFORM_MACOS)
    #include <pthread.h>
#endif

namespace cpptask::details
{
	static void set_current_thread_name(const std::string& name)
	{
#ifdef PLATFORM_WINDOWS
		// Windows 10 version 1607 or later
		std::wstring wname(name.begin(), name.end());
		SetThreadDescription(GetCurrentThread(), wname.c_str());
#elif defined(PLATFORM_LINUX)
		// Thread name is limited to 16 characters on Linux
		std::string truncated_name = name.substr(0, 15);
		pthread_setname_np(pthread_self(), truncated_name.c_str());
#elif defined(PLATFORM_MACOS)
		pthread_setname_np(name.c_str());
#endif
	}

	static std::vector<std::thread> g_worker_threads;
	static std::atomic<bool> g_running{ true };

	static std::mutex g_lock;
	static std::condition_variable g_cv;
	static std::queue<thread_pool::function_t<void()>> g_work_items;

	static std::mutex g_delayed_lock;
	static std::condition_variable g_delayed_cv;
	static std::multimap<std::chrono::steady_clock::time_point, thread_pool::function_t<void()>> g_delayed_work_items;

	static void worker_main(size_t thread_index)
	{
		thread_pool::function_t<void()> work_item;
		set_current_thread_name(std::format("worker #{}", thread_index));

		while (g_running)
		{
			{
				std::unique_lock lock(g_lock);
				g_cv.wait(lock, [] { return !g_work_items.empty() || g_running == false; });
				if (g_running == false)
				{
					break;
				}

				work_item = std::move(g_work_items.front());
				g_work_items.pop();

				lock.unlock();
			}

			std::exchange(work_item, thread_pool::function_t<void()>{})();
		}
	}

	static void delayed_worker_main()
	{
		std::vector<thread_pool::function_t<void()>> actions;
		set_current_thread_name("delayed_worker");

		while (g_running)
		{
			{
				std::unique_lock lock(g_delayed_lock);
				if (g_running == false)
				{
					break;
				}

				g_delayed_cv.wait(lock, [] { return !g_delayed_work_items.empty() || g_running == false; });
				if (g_running == false)
				{
					break;
				}

				auto it = g_delayed_work_items.begin();
				auto until = it->first;
				if (it->first > std::chrono::steady_clock::now())
				{
					g_delayed_cv.wait_until(lock, until);
				}
				if (g_running == false)
				{
					break;
				}

				actions.clear();
				while (!g_delayed_work_items.empty())
				{
					it = g_delayed_work_items.begin();
					if (it->first > std::chrono::steady_clock::now())
					{
						// Not yet.
						break;
					}

					actions.emplace_back(std::move(it->second));
					g_delayed_work_items.erase(it);
				}

				lock.unlock();
			}

			{
				std::unique_lock lock(g_lock);
				for (auto& action : actions)
				{
					g_work_items.emplace(std::move(action));
				}
				g_cv.notify_all();
			}
		}
	}

	static void try_bootstrap()
	{
		static class s_trap_init
		{
		public:
			s_trap_init()
			{
				size_t hardware_concurrency = std::thread::hardware_concurrency();
				for (size_t i = 0; i < hardware_concurrency; ++i)
				{
					g_worker_threads.emplace_back(std::bind(worker_main, i));
				}
				g_worker_threads.emplace_back(delayed_worker_main);
			}

			~s_trap_init() noexcept
			{
				auto lock1 = std::unique_lock{ g_lock };
				auto lock2 = std::unique_lock{ g_delayed_lock };

				g_running = false;

				g_cv.notify_all();
				g_delayed_cv.notify_all();

				lock1.unlock();
				lock2.unlock();

				for (auto& thread : g_worker_threads)
				{
					if (thread.joinable())
					{
						thread.join();
					}
				}
			}
		} _;
	}

	void thread_pool::queue_user_work_item(thread_pool::function_t<void()> continuation)
	{
		try_bootstrap();

		std::unique_lock lock(g_lock);
		g_work_items.emplace(std::move(continuation));
		g_cv.notify_one();
	}

	void thread_pool::queue_delayed_user_work_item(std::chrono::nanoseconds delay, thread_pool::function_t<void()> continuation)
	{
		try_bootstrap();

		auto execute_time = std::chrono::steady_clock::now() + delay;
		std::unique_lock lock(g_delayed_lock);
		g_delayed_work_items.emplace(execute_time, std::move(continuation));
		g_delayed_cv.notify_one();
	}
}