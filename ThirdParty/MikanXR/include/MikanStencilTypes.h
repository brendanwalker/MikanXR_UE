#pragma once

#include "MikanAPIExport.h"
#include "MikanAPITypes.h"
#include "MikanMathTypes.h"
#include "SerializableList.h"
#include "SerializableString.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanStencilTypes.rfkh.h"
#endif

struct STRUCT(Serialization::CodeGenModule("MikanStencilTypes")) MikanStencilQuadInfo
{
	FIELD()
	MikanStencilID stencil_id; // filled in on allocation
	FIELD()
	MikanSpatialAnchorID parent_anchor_id; // if invalid, stencil is in world space
	FIELD()
	MikanTransform relative_transform; // transform relative to parent anchor
	FIELD()
	float quad_width;
	FIELD()
	float quad_height;
	FIELD()
	bool is_double_sided;
	FIELD()
	bool is_disabled;
	FIELD()
	Serialization::String stencil_name;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanStencilQuadInfo_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanStencilTypes")) MikanStencilBoxInfo
{
	FIELD()
	MikanStencilID stencil_id; // filled in on allocation
	FIELD()
	MikanSpatialAnchorID parent_anchor_id; // if invalid, stencil is in world space
	FIELD()
	MikanTransform relative_transform; // transform relative to parent anchor
	FIELD()
	float box_x_size;
	FIELD()
	float box_y_size;
	FIELD()
	float box_z_size;
	FIELD()
	bool is_disabled;
	FIELD()
	Serialization::String stencil_name;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanStencilBoxInfo_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanStencilTypes")) MikanStencilModelInfo
{
	FIELD()
	MikanStencilID stencil_id; // filled in on allocation
	FIELD()
	MikanSpatialAnchorID parent_anchor_id; // if invalid, stencil is in world space
	FIELD()
	MikanTransform relative_transform; // transform relative to parent anchor
	FIELD()
	bool is_disabled;
	FIELD()
	Serialization::String stencil_name;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanStencilModelInfo_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanStencilTypes")) MikanTriagulatedMesh
{
	FIELD()
	Serialization::List<MikanVector3f> vertices;
	FIELD()
	Serialization::List<MikanVector3f> normals;
	FIELD()
	Serialization::List<MikanVector2f> texels;
	FIELD()
	Serialization::List<int> indices;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanTriagulatedMesh_GENERATED
	#endif
};

struct STRUCT(Serialization::CodeGenModule("MikanStencilTypes")) MikanStencilModelRenderGeometry
{
	FIELD()
	Serialization::List<MikanTriagulatedMesh> meshes;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanStencilModelRenderGeometry_GENERATED
	#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanStencilTypes_GENERATED
#endif