#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanMathTypes.h"
#include "SerializableString.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanScriptEvents.rfkh.h"
#endif

struct STRUCT(Serialization::CodeGenModule("MikanScriptEvents")) MikanScriptMessagePostedEvent : public MikanEvent
{
	MikanScriptMessagePostedEvent()
	{
		MIKAN_EVENT_TYPE_INFO_INIT(MikanScriptMessagePostedEvent)
	}

	FIELD()
	Serialization::String message;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanScriptMessagePostedEvent_GENERATED
	#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanScriptEvents_GENERATED
#endif