#pragma once

#include "MikanAPIExport.h"
#include "MikanTypeFwd.h"
#include "MikanCoreTypes.h"
#include "SerializableString.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanClientTypes.rfkh.h"
#endif // MIKANAPI_REFLECTION_ENABLED

#include <string>

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanClientTypes")) MikanClientInfo
{
	FIELD()
	MikanClientAPIVersion apiVersion;
	FIELD()
	Serialization::String clientId;
	FIELD()
	Serialization::String engineName;
	FIELD()
	Serialization::String engineVersion;
	FIELD()
	Serialization::String applicationName;
	FIELD()
	Serialization::String applicationVersion;
	FIELD()
	Serialization::String xrDeviceName;
	FIELD()
	MikanClientGraphicsApi graphicsAPI;
	FIELD()
	bool supportsRGB24;
	FIELD()
	bool supportsRGBA32;
	FIELD()
	bool supportsBGRA32;
	FIELD()
	bool supportsDepth;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanClientInfo_GENERATED
	#endif // MIKANAPI_REFLECTION_ENABLED
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanClientTypes_GENERATED
#endif // MIKANAPI_REFLECTION_ENABLED