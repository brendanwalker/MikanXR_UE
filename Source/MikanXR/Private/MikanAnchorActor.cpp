#include "MikanAnchorActor.h"
#include "MikanAnchorTypes.h"
#include "DrawDebugHelpers.h"

AMikanAnchorActor::AMikanAnchorActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AMikanAnchorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Draw coordinate axes to visualize anchor position and orientation
	const float AxisLength = 50.f;
	DrawDebugCoordinateSystem(GetWorld(), GetActorLocation(), GetActorRotation(), AxisLength);
}

