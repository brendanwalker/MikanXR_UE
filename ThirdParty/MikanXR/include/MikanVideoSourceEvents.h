#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanMathTypes.h"
#include "SerializableString.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanVideoSourceEvents.rfkh.h"
#endif

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVideoSourceEvents")) MikanVideoSourceOpenedEvent
	: public MikanEvent
{
	MikanVideoSourceOpenedEvent(){MIKAN_EVENT_TYPE_INFO_INIT(MikanVideoSourceOpenedEvent)}

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVideoSourceOpenedEvent_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVideoSourceEvents")) MikanVideoSourceClosedEvent
	: public MikanEvent
{
	MikanVideoSourceClosedEvent(){MIKAN_EVENT_TYPE_INFO_INIT(MikanVideoSourceClosedEvent)}

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVideoSourceClosedEvent_GENERATED
#endif
};

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanVideoSourceEvents")) MikanVideoSourceModeChangedEvent
	: public MikanEvent
{
	MikanVideoSourceModeChangedEvent(){MIKAN_EVENT_TYPE_INFO_INIT(MikanVideoSourceModeChangedEvent)}

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVideoSourceModeChangedEvent_GENERATED
#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanVideoSourceEvents_GENERATED
#endif