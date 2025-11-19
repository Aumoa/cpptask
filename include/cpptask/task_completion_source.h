// Copyright 2020-2025 Aumoa. All right reserved.

#pragma once

#include "cpptask/details/shared_task.h"
#include <utility>
#include <memory>

namespace cpptask
{
	template<class T = void>
	class task_completion_source
	{
	private:
		std::shared_ptr<details::shared_task<T>> _task;

	private:
		task_completion_source(std::in_place_t)
		{
			_task = std::make_shared<details::shared_task<T>>();
			_task->transit_to_running();
		}

	public:
		task_completion_source() = default;

		bool is_valid() const noexcept
		{
			return _task != nullptr;
		}

		template<class... U>
		void set_result(U&&... args) const requires
			(std::same_as<T, void> && sizeof...(U) == 0) ||
			(!std::same_as<T, void> && std::constructible_from<T, U...>)
		{
			_task->set_result(std::forward<U>(args)...);
		}

		bool try_cancel() const noexcept
		{
			return _task->try_cancel();
		}

		bool try_set_exception(std::exception_ptr except) const noexcept
		{
			return _task->try_set_exception(except);
		}

		task<T> get_task() const noexcept
		{
			return task<T>(_task);
		}

	public:
		static task_completion_source create()
		{
			return task_completion_source(std::in_place);
		}
	};
}