#pragma once

#include "MikanCoreTypes.h"
#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanPropertyTypes.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanPropertyEvents.rfkh.h"
#endif

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanPropertyEvents")) MikanPropertyUpdateEvent
	: public MikanEvent
{
	MikanPropertyUpdateEvent(){MIKAN_EVENT_TYPE_INFO_INIT(MikanPropertyUpdateEvent)}

	FIELD() MikanPropertyValue propertyValue;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanPropertyUpdateEvent_GENERATED
#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanPropertyEvents_GENERATED
#endif
