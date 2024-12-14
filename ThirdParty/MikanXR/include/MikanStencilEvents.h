#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanMathTypes.h"
#include "SerializableString.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanStencilEvents.rfkh.h"
#endif

struct STRUCT(Serialization::CodeGenModule("MikanStencilEvents")) MikanStencilNameUpdateEvent : public MikanEvent
{
	MikanStencilNameUpdateEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanStencilNameUpdateEvent)
	}

	FIELD()
	MikanStencilID stencil_id;
	FIELD()
	Serialization::String stencil_name;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanStencilNameUpdateEvent_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanStencilEvents")) MikanStencilPoseUpdateEvent : public MikanEvent
{
	MikanStencilPoseUpdateEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanStencilPoseUpdateEvent)
	}

	FIELD()
	MikanTransform transform;
	FIELD()
	MikanStencilID stencil_id;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanStencilPoseUpdateEvent_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanStencilEvents")) MikanQuadStencilListUpdateEvent : public MikanEvent
{
	MikanQuadStencilListUpdateEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanQuadStencilListUpdateEvent)
	}

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanQuadStencilListUpdateEvent_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanStencilEvents")) MikanBoxStencilListUpdateEvent : public MikanEvent
{
	MikanBoxStencilListUpdateEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanBoxStencilListUpdateEvent)
	}

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanBoxStencilListUpdateEvent_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanStencilEvents")) MikanModelStencilListUpdateEvent : public MikanEvent
{
	MikanModelStencilListUpdateEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanModelStencilListUpdateEvent)
	}

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanModelStencilListUpdateEvent_GENERATED
	#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanStencilEvents_GENERATED
#endif