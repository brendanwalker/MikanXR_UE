#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanComponentTypes.h"
#include "MikanMathTypes.h"
#include "SerializableList.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanTransformTypes.rfkh.h"
#endif

struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanTransformTypes")) MikanTransformComponentValues
	: public MikanComponentValues
{
	FIELD() MikanTransformID parent_transform_id= INVALID_MIKAN_ID;
	FIELD() MikanVector3f relative_scale;
	FIELD() MikanQuatf relative_quaternion;
	FIELD() MikanVector3f relative_position;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanTransformComponentValues_GENERATED
#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanTransformTypes_GENERATED
#endif