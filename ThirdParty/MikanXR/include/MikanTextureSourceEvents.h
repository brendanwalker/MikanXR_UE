#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanMathTypes.h"
#include "SerializableString.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanTextureSourceEvents.rfkh.h"
#endif

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanTextureSourceEvents")) MikanTextureSourceOpenedEvent
	: public MikanEvent
{
	MikanTextureSourceOpenedEvent(){MIKAN_EVENT_TYPE_INFO_INIT(MikanTextureSourceOpenedEvent)}

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanTextureSourceOpenedEvent_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanTextureSourceEvents")) MikanTextureSourceClosedEvent
	: public MikanEvent
{
	MikanTextureSourceClosedEvent(){MIKAN_EVENT_TYPE_INFO_INIT(MikanTextureSourceClosedEvent)}

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanTextureSourceClosedEvent_GENERATED
#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanTextureSourceEvents_GENERATED
#endif