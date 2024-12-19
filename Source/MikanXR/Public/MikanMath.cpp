#include "MikanMath.h"
#include "Math/Matrix.h"
#include "Math/Quat.h"
#include "Math/Transform.h"

FMatrix FMikanMath::MikanMatrix4fToFMatrix(const MikanMatrix4f& xform)
{
	return FMatrix(
		FPlane(xform.x0, xform.x1, xform.x2, xform.x3),
		FPlane(xform.z0, xform.z1, xform.z2, xform.z3),
		FPlane(xform.y0, xform.y1, xform.y2, xform.y3),
		FPlane(xform.w2, xform.w0, xform.w1, xform.w3));
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