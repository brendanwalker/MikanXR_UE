#pragma once

#include "MikanAPIExport.h"
#include "SerializationProperty.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanMathTypes.rfkh.h"
#endif

//-- constants -----
/// Conversion factor to go from meters to centimeters
#define MIKAN_METERS_TO_CENTIMETERS  100.f

/// Conversion factor to go from centimeters to meters
#define MIKAN_CENTIMETERS_TO_METERS  0.01f

/// A 3D vector with double components.
struct STRUCT(Serialization::CodeGenModule("MikanMathTypes")) MikanVector3d
{
	FIELD()
	double x;
	FIELD()
	double y;
	FIELD()
	double z;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVector3d_GENERATED
	#endif
};

/// A 2D vector with float components.
struct STRUCT(Serialization::CodeGenModule("MikanMathTypes")) MikanVector2f
{
	FIELD()
	float x;
	FIELD()
	float y;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVector2f_GENERATED
	#endif
};

/// A 3D vector with float components.
struct STRUCT(Serialization::CodeGenModule("MikanMathTypes")) MikanVector3f
{
	FIELD()
    float x;
	FIELD()
	float y;
	FIELD()
	float z;


	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanVector3f_GENERATED
	#endif
};

/// A 3-tuple of Euler angles with float components.
struct STRUCT(Serialization::CodeGenModule("MikanMathTypes")) MikanRotator3f
{
	FIELD()
	float x_angle;
	FIELD()
	float y_angle;
	FIELD()
	float z_angle;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanRotator3f_GENERATED
	#endif
};

/** A 4x4 matrix with float components
    storage is column major order:

    | x0 y0 z0 w0 |
    | x1 y1 z1 w1 |
    | x2 y2 z2 w2 |
    | x3 y3 z3 w3 |
 */
struct STRUCT(Serialization::CodeGenModule("MikanMathTypes")) MikanMatrix4f
{
	FIELD()
	float x0;
	FIELD()
	float x1;
	FIELD()
	float x2;
	FIELD()
	float x3;
	FIELD()
	float y0;
	FIELD()
	float y1;
	FIELD()
	float y2;
	FIELD()
	float y3;
	FIELD()
	float z0;
	FIELD()
	float z1;
	FIELD()
	float z2;
	FIELD()
	float z3;
	FIELD()
	float w0;
	FIELD()
	float w1;
	FIELD()
	float w2;
	FIELD()
	float w3;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanMatrix4f_GENERATED
	#endif
};

/** A 3x3 matrix with double components
    storage is column major order:

    | x0 y0 z0 |
    | x1 y1 z1 |
    | x2 y2 z2 |
 */
struct STRUCT(Serialization::CodeGenModule("MikanMathTypes")) MikanMatrix3d
{
	FIELD()
	double x0;
	FIELD()
	double x1;
	FIELD()
	double x2;
	FIELD()
	double y0;
	FIELD()
	double y1;
	FIELD()
	double y2;
	FIELD()
	double z0;
	FIELD()
	double z1;
	FIELD()
	double z2;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanMatrix3d_GENERATED
	#endif
};

/** A 4x3 matrix with double components
    storage is column major order:

    | x0 y0 z0 w0|
    | x1 y1 z1 w1|
    | x2 y2 z2 w2|
 */
struct STRUCT(Serialization::CodeGenModule("MikanMathTypes")) MikanMatrix4x3d
{
	FIELD()
	double x0;
	FIELD()
	double x1;
	FIELD()
	double x2;
	FIELD()
	double x3;
	FIELD()
	double y0;
	FIELD()
	double y1;
	FIELD()
	double y2;
	FIELD()
	double y3;
	FIELD()
	double z0;
	FIELD()
	double z1;
	FIELD()
	double z2;
	FIELD()
	double z3;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanMatrix4x3d_GENERATED
	#endif
};

/** A 4x4 matrix with double components
    storage is column major order:

    | x0 y0 z0 w0 |
    | x1 y1 z1 w1 |
    | x2 y2 z2 w2 |
    | x3 y3 z3 w3 |
 */
struct STRUCT(Serialization::CodeGenModule("MikanMathTypes")) MikanMatrix4d
{
	FIELD()
	double x0;
	FIELD()
	double x1;
	FIELD()
	double x2;
	FIELD()
	double x3;
	FIELD()
	double y0;
	FIELD()
	double y1;
	FIELD()
	double y2;
	FIELD()
	double y3;
	FIELD()
	double z0;
	FIELD()
	double z1;
	FIELD()
	double z2;
	FIELD()
	double z3;
	FIELD()
	double w0;
	FIELD()
	double w1;
	FIELD()
	double w2;
	FIELD()
	double w3;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanMatrix4d_GENERATED
	#endif
};

/// A double-precision quaternion rotation.
struct STRUCT(Serialization::CodeGenModule("MikanMathTypes")) MikanQuatf
{
	FIELD()
	float w;
	FIELD()
	float x;
	FIELD()
	float y;
	FIELD()
	float z;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanQuatf_GENERATED
	#endif
};

/// A double-precision quaternion rotation.
struct STRUCT(Serialization::CodeGenModule("MikanMathTypes")) MikanQuatd
{
	FIELD()
    double w;
	FIELD()
	double x;
	FIELD()
	double y;
	FIELD()
	double z;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanQuatd_GENERATED
	#endif
};

/// A single-precision Scale-Rotation-Translation transform.
struct STRUCT(Serialization::CodeGenModule("MikanMathTypes")) MikanTransform
{
	FIELD()
	MikanVector3f scale;
	FIELD()
	MikanQuatf rotation;
	FIELD()
	MikanVector3f position;

	#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanTransform_GENERATED
	#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanMathTypes_GENERATED
#endif