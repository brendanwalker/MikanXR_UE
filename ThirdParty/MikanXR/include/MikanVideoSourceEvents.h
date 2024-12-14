#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanMathTypes.h"
#include "SerializableString.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanVideoSourceEvents.rfkh.h"
#endif

struct STRUCT(Serialization::CodeGenModule("MikanVideoSourceEvents")) MikanVideoSourceOpenedEvent : 
	public MikanEvent
{
	MikanVideoSourceOpenedEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanVideoSourceOpenedEvent)
	}

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVideoSourceOpenedEvent_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanVideoSourceEvents")) MikanVideoSourceClosedEvent : 
	public MikanEvent
{
	MikanVideoSourceClosedEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanVideoSourceClosedEvent)
	}

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVideoSourceClosedEvent_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanVideoSourceEvents")) MikanVideoSourceNewFrameEvent : 
	public MikanEvent
{
	MikanVideoSourceNewFrameEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanVideoSourceNewFrameEvent)
	}

	FIELD()
	MikanVector3f cameraForward;
	FIELD()
	MikanVector3f cameraUp;
	FIELD()
	MikanVector3f cameraPosition;
	FIELD()
	uint64_t frame;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVideoSourceNewFrameEvent_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanVideoSourceEvents")) MikanVideoSourceAttachmentChangedEvent : 
	public MikanEvent
{
	MikanVideoSourceAttachmentChangedEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanVideoSourceAttachmentChangedEvent)
	}

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVideoSourceAttachmentChangedEvent_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanVideoSourceEvents")) MikanVideoSourceIntrinsicsChangedEvent : 
	public MikanEvent
{
	MikanVideoSourceIntrinsicsChangedEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanVideoSourceIntrinsicsChangedEvent)
	}

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVideoSourceIntrinsicsChangedEvent_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanVideoSourceEvents")) MikanVideoSourceModeChangedEvent : 
	public MikanEvent
{
	MikanVideoSourceModeChangedEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanVideoSourceModeChangedEvent)
	}

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVideoSourceModeChangedEvent_GENERATED
	#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanVideoSourceEvents_GENERATED
#endif