#pragma once

#include "Math/Matrix.h"
#include "MikanMathTypes.h"

class FMikanMath
{
public:
	static FMatrix MikanMatrix4fToFMatrix(const MikanMatrix4f& xform);
	static FTransform MikanTransformToFTransform(const MikanTransform& xform, const float MetersToUU);
	static FVector MikanVector3fToFVector(const MikanVector3f& v);
	static FVector2D MikanVector2fToFVector2D(const MikanVector2f& v);
	static FQuat MikanQuatToFQuat(const MikanQuatf& q);
	static MikanVector3f FVectorToMikanVector3f(const FVector& v);
};
