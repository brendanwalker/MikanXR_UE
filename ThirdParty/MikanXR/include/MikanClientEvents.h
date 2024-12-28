#pragma once

#include "MikanCoreTypes.h"
#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanMathTypes.h"
#include "SerializableString.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanClientEvents.rfkh.h"
#endif

struct STRUCT(Serialization::CodeGenModule("MikanClientEvents")) MikanConnectedEvent : public MikanEvent
{
	MikanConnectedEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanConnectedEvent)
	}

	FIELD()
	MikanClientAPIVersion serverVersion;

	FIELD()
	MikanClientAPIVersion minClientVersion;

	FIELD()
	bool isClientCompatible;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanConnectedEvent_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanClientEvents")) MikanDisconnectedEvent : public MikanEvent
{
	MikanDisconnectedEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanDisconnectedEvent)
	}

	FIELD()
	MikanDisconnectCode code;

	FIELD()
	Serialization::String reason;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanDisconnectedEvent_GENERATED
	#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanClientEvents_GENERATED
#endif