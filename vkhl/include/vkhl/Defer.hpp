#pragma once

#ifndef VKHL_DEFER_HPP
#define VKHL_DEFER_HPP

#include <utility>
#include <optional>
#include <vulkan/vulkan_core.h>

namespace vkhl
{
	template<typename ArgT, typename FuncT = void(*)(ArgT)>
	struct FuncArgBinding
	{
		FuncT func;
		ArgT arg;

		void operator()() {
			func(arg);
		}
	};

#if __cpp_concepts // Concepts allows, function cannot take arguments
	template<typename FuncT>
	concept CallableNoArgs = requires(FuncT func) {
		func();
	};

	// No arguments
	template<CallableNoArgs FuncT>
#else // No concepts, function still cannot take arguments, but no way to enforce it nicely
	// No arguments
	template<typename FuncT>
#endif
	class Defer
	{
	public:
		Defer() = default;
		Defer(const Defer&) = delete;
		Defer(Defer&& rhs)
		{
			std::swap(m_func, rhs.m_func);
		}

		Defer(const FuncT& func)
			:m_func(func)
		{
		}

		~Defer() { Destroy(); }

		void Cancel()
		{
			m_func = std::nullopt;
		}

		void Destroy()
		{
			if (m_func.has_value())
			{
				m_func.value()();
				m_func = std::nullopt;
			}
		}

	private:
		std::optional<FuncT> m_func = std::nullopt;
	};
}

#endif
