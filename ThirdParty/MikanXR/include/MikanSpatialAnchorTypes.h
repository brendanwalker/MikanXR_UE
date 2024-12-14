#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanMathTypes.h"
#include "SerializableList.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanSpatialAnchorTypes.rfkh.h"
#endif

struct STRUCT(Serialization::CodeGenModule("MikanSpatialAnchorTypes")) MikanSpatialAnchorInfo
{
	FIELD()
	MikanSpatialAnchorID anchor_id;
	FIELD()
	MikanTransform world_transform; // Transform in tracking system space
	FIELD()
	Serialization::String anchor_name;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanSpatialAnchorInfo_GENERATED
	#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanSpatialAnchorTypes_GENERATED
#endif