#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MikanUtils.generated.h"

UCLASS()
class MIKANXR_API UMikanUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintPure, Category = "MikanXR", meta = (WorldContext = "WorldContextObject"))
	static bool AnyMikanRenderableOverlapsCone(
		class AMikanClient* MikanClient, 
		const FVector& Origin, 
		const FVector& Direction, 
		float HalfAngleDegrees);
};