#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "MikanClientTypes.h"
#include "MikanAnchorComponent.generated.h"

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class MIKANXR_API UMikanAnchorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UMikanAnchorComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString AnchorName;

	UFUNCTION(BlueprintPure)
	class AMikanScene* GetParentScene() const;

	void FetchAnchorInfo();
	void UpdateSceneTransform();

	MikanSpatialAnchorID GetAnchorId() const { return AnchorId; }

protected:
	MikanSpatialAnchorID AnchorId= INVALID_MIKAN_ID;
};
