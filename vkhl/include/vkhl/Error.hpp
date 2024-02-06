#pragma once

#ifndef VKHL_ERROR_HPP
#define VKHL_ERROR_HPP

#include <iostream>
#include <cstdarg>

#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>

namespace vkhl
{

	using PrintFunc = void(*)(const char* format, va_list args);

	void DefaultPrintFunc(const char* format, va_list args) { vprintf(format, args); }

	// Set to nullptr to disable error messages
	PrintFunc g_printErrorFunc = DefaultPrintFunc;

	// Set to nullptr to disable warning messages
	PrintFunc g_printWarningFunc = DefaultPrintFunc;

	void PrintError(const char* format, ...)
	{
		std::va_list varargs;
		va_start(varargs, format);

		if (g_printErrorFunc)
			g_printErrorFunc(format, varargs);
	}

	void PrintWarning(const char* format, ...)
	{
		std::va_list varargs;
		va_start(varargs, format);

		if (g_printWarningFunc)
			g_printWarningFunc(format, varargs);
	}

	// Prints Error if unhandled
	class SmartResult
	{
	public:
		SmartResult() = default;
		SmartResult(const SmartResult&) = default;
		SmartResult& operator=(const SmartResult&) = default;

		SmartResult(VkResult result)
			:m_result(result)
		{}

		SmartResult& operator=(VkResult result)
		{
			m_result = result;
		}

		~SmartResult()
		{
			if (m_result < 0)
				PrintError("Unhandled Vulkan error: %s\n", string_VkResult(m_result));
		}

		VkResult Get() { return m_result; }
		void Reset() { m_result = VK_SUCCESS; }

		VkResult GetAndReset()
		{
			auto oldResult = m_result;
			m_result = VK_SUCCESS;
			return oldResult;
		}

	private:
		VkResult m_result;
	};
}

#endif
