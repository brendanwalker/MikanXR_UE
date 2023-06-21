/**
\file
*/

#ifndef __MIKAN_MATH_TYPES_H
#define __MIKAN_MATH_TYPES_H
//cut_before

/**
\brief Geometrical data structures used by the Client API
\defgroup Mikan Math Data Types
\addtogroup MikanMath
@{
*/

//-- constants -----
/// Conversion factor to go from meters to centimeters
#define MIKAN_METERS_TO_CENTIMETERS  100.f

/// Conversion factor to go from centimeters to meters
#define MIKAN_CENTIMETERS_TO_METERS  0.01f

/// A 3D vector with double components.
typedef struct
{
    double x, y, z;
} MikanVector3d;

/// A 3D vector with float components.
typedef struct
{
    float x, y, z;
} MikanVector3f;

/// A 3-tuple of Euler angles with float components.
typedef struct
{
	float x_angle, y_angle, z_angle;
} MikanRotator3f;

/** A 4x4 matrix with float components
    storage is column major order:

    | x0 y0 z0 w0 |
    | x1 y1 z1 w1 |
    | x2 y2 z2 w2 |
    | x3 y3 z3 w3 |
 */
typedef struct
{
	float x0;
	float x1;
	float x2;
	float x3;
	float y0;
	float y1;
	float y2;
	float y3;
	float z0;
	float z1;
	float z2;
	float z3;
	float w0;
	float w1;
	float w2;
	float w3;
} MikanMatrix4f;

/** A 3x3 matrix with double components
    storage is column major order:

    | x0 y0 z0 |
    | x1 y1 z1 |
    | x2 y2 z2 |
 */
typedef struct
{
	double x0;
	double x1;
	double x2;
	double y0;
	double y1;
	double y2;
	double z0;
	double z1;
	double z2;
} MikanMatrix3d;

/** A 4x3 matrix with double components
    storage is column major order:

    | x0 y0 z0 w0|
    | x1 y1 z1 w1|
    | x2 y2 z2 w2|
 */
typedef struct
{
	double x0;
	double x1;
	double x2;
	double x3;
	double y0;
	double y1;
	double y2;
	double y3;
	double z0;
	double z1;
	double z2;
	double z3;
} MikanMatrix4x3d;

/** A 4x4 matrix with double components
    storage is column major order:

    | x0 y0 z0 w0 |
    | x1 y1 z1 w1 |
    | x2 y2 z2 w2 |
    | x3 y3 z3 w3 |
 */
typedef struct
{
	double x0;
	double x1;
	double x2;
	double x3;
	double y0;
	double y1;
	double y2;
	double y3;
	double z0;
	double z1;
	double z2;
	double z3;
	double w0;
	double w1;
	double w2;
	double w3;
} MikanMatrix4d;

/// A double-precision quaternion rotation.
typedef struct
{
	float w, x, y, z;
} MikanQuatf;

/// A double-precision quaternion rotation.
typedef struct
{
    double w, x, y, z;
} MikanQuatd;

/// A single-precision Scale-Rotation-Translation transform.
typedef struct
{
	MikanVector3f scale;
	MikanQuatf rotation;
	MikanVector3f position;
} MikanTransform;

/**
@}
*/

//cut_after
#endif
