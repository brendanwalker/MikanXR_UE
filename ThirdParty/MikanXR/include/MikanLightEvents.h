#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanLightTypes.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanLightEvents.rfkh.h"
#endif

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanLightEvents")) MikanLightDMXDataChangedEvent
	: public MikanEvent
{
	MikanLightDMXDataChangedEvent(){MIKAN_EVENT_TYPE_INFO_INIT(MikanLightDMXDataChangedEvent)}

	FIELD() MikanDMXData dmx_data;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanLightDMXDataChangedEvent_GENERATED
#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanLightEvents_GENERATED
#endif
