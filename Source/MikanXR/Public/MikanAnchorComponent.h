#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "MikanSpatialAnchorTypes.h"
#include "MikanAnchorComponent.generated.h"

UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class MIKANXR_API UMikanAnchorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UMikanAnchorComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintReadOnly)
	int32 AnchorId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString AnchorName;

	UFUNCTION(BlueprintPure)
	class AMikanScene* GetParentScene() const;

	void ApplyAnchorInfo(
		MikanSpatialAnchorID InAnchorId,
		const MikanTransform& RelativeTransform);
	void ApplyAnchorTransform(const MikanTransform& RelativeTransform);

#if WITH_EDITOR  
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
