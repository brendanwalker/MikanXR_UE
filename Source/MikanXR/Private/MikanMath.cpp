#include "MikanMath.h"
#include "MikanMathTypes.h"
#include "MikanVideoSourceTypes.h"
#include "Math/Matrix.h"
#include "Math/Quat.h"
#include "Math/Transform.h"

FMatrix FMikanMath::MikanMatrix4fToFMatrix(const MikanMatrix4f& xform)
{
	// Mikan stores a column-major, right-handed (+Y up) transform where each
	// lettered column is a basis vector image:
	//   X axis = (x0,x1,x2), Y axis = (y0,y1,y2), Z axis = (z0,z1,z2), translation = (w0,w1,w2).
	//
	// Unreal's FMatrix is row-major with a row-vector convention (v' = v * M), so the
	// basis vectors live in the *rows*. Converting requires both:
	//   1) the right-handed -> left-handed change of basis C (swap Y/Z), applied as C*M*C^-1, and
	//   2) a transpose from column-vector to row-vector layout.
	//
	// The net effect: Unreal row N is the Y/Z-swapped Mikan column for that axis, and the
	// numeric (row) indices 1 and 2 swap within each row to apply C on the right.
	return FMatrix(
		FPlane(xform.x0, xform.x2, xform.x1, xform.x3),	// Unreal X axis  <- Mikan X column
		FPlane(xform.z0, xform.z2, xform.z1, xform.z3),	// Unreal Y axis  <- Mikan Z column
		FPlane(xform.y0, xform.y2, xform.y1, xform.y3),	// Unreal Z axis  <- Mikan Y column
		FPlane(xform.w0, xform.w2, xform.w1, xform.w3));	// translation
}

FTransform FMikanMath::MikanTransformToFTransform(
	const MikanTransform& xform,
	const float MetersToUU)
{
	const MikanVector3f& s= xform.scale;
	const FVector Location= MikanVector3fToFVector(xform.position) * MetersToUU;

	return FTransform(
		MikanQuatToFQuat(xform.rotation),
		Location,
		FVector(s.x, s.z, s.y));
}

FTransform FMikanMath::MikanTransformToFTransform(
	const MikanVector3f& position,
	const MikanQuatf& rotation,
	const MikanVector3f& scale,
	const float MetersToUU)
{
	const FVector Location= MikanVector3fToFVector(position) * MetersToUU;

	return FTransform(
		MikanQuatToFQuat(rotation),
		Location,
		FVector(scale.x, scale.z, scale.y));
}

FQuat FMikanMath::MikanQuatToFQuat(const MikanQuatf& q)
{
	return FQuat(-q.x, -q.z, -q.y, q.w);
}

MikanVector3f FMikanMath::FVectorToMikanVector3f(const FVector& v)
{
	return {(float)v.X, (float)v.Z, (float)v.Y};
}

FVector FMikanMath::MikanVector3fToFVector(const MikanVector3f& v)
{
	return FVector(v.x, v.z, v.y);
}

FVector2D FMikanMath::MikanVector2fToFVector2D(const MikanVector2f& v)
{
	return FVector2D(v.x, v.y);
}

void FMikanMath::ExtractCameraIntrinsicMatrixParameters(
	const MikanMatrix3d& IntrinsicMatrix,
	float& OutFocalLengthX,
	float& OutFocalLengthY,
	float& OutPrincipalPointX,
	float& OutPrincipalPointY)
{
	OutFocalLengthX = IntrinsicMatrix.x0;
	OutFocalLengthY = IntrinsicMatrix.y1;
	OutPrincipalPointX = IntrinsicMatrix.z0;
	OutPrincipalPointY = IntrinsicMatrix.z1;
}

FMatrix FMikanMath::ComputeProjectionMatFromCameraIntrinsics(
	const MikanMonoIntrinsics& intrinsics,
	const float MetersToUU,
	float* OutNearClippingPlaneUU,
	float* OutFarClippingPlaneUU)
{
	// Extract focal lengths and principal point from the camera intrinsics
	float Alpha, Beta, U0, V0;
	ExtractCameraIntrinsicMatrixParameters(
		intrinsics.undistorted_camera_matrix,
		Alpha,	// focal length x
		Beta,	// focal length y
		U0,		// principal point x (usually image center x)
		V0);	// principal point y (usually image center y)

	// Get the image dimensions of the camera
	float Width = intrinsics.pixel_width;
	float Height = intrinsics.pixel_height;

	// Get the near and far clipping planes, used for the mapping from
	// world-space z-coordinate into the depth coordinate for Unreal Engine.
	// NOTE: intrinsics are in meters so we convert to Unreal Units (UU)
	float N = intrinsics.znear * MetersToUU;
	float F = intrinsics.zfar * MetersToUU;

	// Optionally return out the near and far clipping planes
	if (OutNearClippingPlaneUU != nullptr)
	{
		*OutNearClippingPlaneUU = N;
	}
	if (OutFarClippingPlaneUU != nullptr)
	{
		*OutFarClippingPlaneUU = F;
	}

	return ComputeProjectionMatFromCameraIntrinsics(
		Width,
		Height,
		Alpha,
		Beta,
		U0,
		V0,
		N,		// near clipping plane in Unreal Units (UU)
		F);		// far clipping plane in Unreal Units (UU)
}

FMatrix FMikanMath::ComputeProjectionMatFromCameraIntrinsics(
	const float Width,	// image width in pixels
	const float Height,	// image height in pixels
	const float Alpha,	// focal length x
	const float Beta,	// focal length y
	const float U0,		// principal point x (usually image center x)
	const float V0,		// principal point y (usually image center y)
	const float N,		// near clipping plane in Unreal Units (UU)
	const float F)		// far clipping plane in Unreal Units (UU)
{

	// This perspective matrix is a tweaked version of TReversedZPerspectiveMatrix
	// which is used in Unreal Engine to render the scene (See PerspectiveMatrix.h) 
	// 
	// FMatrix(
	//	FPlane(1.0f / FMath::Tan(HalfFOVX),                        0.0f,                          0.0f, 0.0f),
	//	FPlane(                       0.0f, 1.0f / FMath::Tan(HalfFOVY),                          0.0f, 0.0f),
	//	FPlane(                       0.0f,                        0.0f,          MinZ / (MinZ - MaxZ), 1.0f),
	//	FPlane(                       0.0f,                        0.0f, -MaxZ * MinZ / (MinZ - MaxZ)), 0.0f));
	//
	// But with the following tweaks:
	// 
	// The usual FoV angle expressions are replaced with the focal length expressions.
	// 1.f / tan(HalfFOVX) ~~> (2.f * alpha) / width
	// 1.f / tan(HalfFOVY) ~~> (2.f * beta) / height
	// 
 	// And the frustum has to be shifted slightly when the principal point is not at the image center.
	// [(2.f*u0/width) - 1.f] and [1.f - (2*v0/height)] 
	// These become 0 when the principal point is at the image center.
	// 
	// The z-coordinate remapping math remains the same.
	FMatrix Projection(
		FPlane((2.f * Alpha) / Width,                  0.0f,             0.0f, 0.0f),
		FPlane(                 0.0f, (2.f * Beta) / Height,             0.0f, 0.0f),
		FPlane( (2.f*U0/Width) - 1.f,   1.f - (2*V0/Height),      N / (N - F), 1.0f),
		FPlane(                 0.0f,                  0.0f, -F * N / (N - F), 0.0f));

	return Projection;
}