#pragma once

#include "SharedTextureFwd.h"
#include "SharedTextureExport.h"

#include <string>

enum class SharedTextureType : int
{
	UNKNOWN= -1,

	COLOR,
	DEPTH,
	SHADOW,
};

MIKAN_SHAREDTEXTURE_FUNC(bool) makeSpoutSenderName(const std::string prefix, MikanCameraID camera_id,
												   SharedTextureType buffer_type, std::string& outSenderName);