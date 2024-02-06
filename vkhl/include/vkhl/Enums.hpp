#pragma once

#ifndef VKHL_ENUMS_HPP
#define VKHL_ENUMS_HPP

namespace vkhl
{
	enum FeatureRequirement
	{
		RequireFeature = 0, // Errors if not available
		RequestFeature = 1, // Warns if not available
	};
}

#endif
