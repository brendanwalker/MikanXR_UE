#pragma once

#include "MikanMathTypes.h"
#include "Math/Matrix.h"

class FMikanMath
{
public:
	static constexpr float kMetersToCentimeters = 100.f;
	static constexpr float kMetersToMillimeters = 1000.f;

	static constexpr float kCentimetersToMeters = 1.f / kMetersToCentimeters;
	static constexpr float kMillimetersToMeters = 1.f / kMetersToMillimeters;

	static FMatrix MikanMatrix4fToFMatrix(const MikanMatrix4f& xform);
	static FTransform MikanTransformToFTransform(
		const MikanTransform& xform, 
		const float MetersToUU);
	static FTransform MikanTransformToFTransform(
		const MikanVector3f& position,
		const MikanQuatf& rotation,
		const MikanVector3f& scale,
		const float MetersToUU);
	static FVector MikanVector3fToFVector(const MikanVector3f& v);
	static FVector2D MikanVector2fToFVector2D(const MikanVector2f& v);
	static FQuat MikanQuatToFQuat(const MikanQuatf& q);
	static MikanVector3f FVectorToMikanVector3f(const FVector& v);
	static void ExtractCameraIntrinsicMatrixParameters(
		const MikanMatrix3d& IntrinsicMatrix,
		float& OutFocalLengthX,
		float& OutFocalLengthY,
		float& OutPrincipalPointX,
		float& OutPrincipalPointY);
	static FMatrix ComputeProjectionMatFromCameraIntrinsics(
		const struct MikanMonoIntrinsics& intrinsics,
		const float MetersToUU,
		float* OutNearClippingPlaneUU,
		float* OutFarClippingPlaneUU);
	static FMatrix ComputeProjectionMatFromCameraIntrinsics(
		const float Width,	// image width in pixels
		const float Height,	// image height in pixels
		const float Alpha,	// focal length x
		const float Beta,	// focal length y
		const float U0,		// principal point x (usually image center x)
		const float V0,		// principal point y (usually image center y)
		const float N,		// near clipping plane in Unreal Units (UU)
		const float F);		// far clipping plane in Unreal Units (UU)
};
