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

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString AnchorName;

	MikanSpatialAnchorID GetAnchorId() const { return AnchorId; }

protected:
	UFUNCTION()
	void NotifyMikanConnected();
	void FindAnchorInfo();

	MikanSpatialAnchorID AnchorId= INVALID_MIKAN_ID;
};
