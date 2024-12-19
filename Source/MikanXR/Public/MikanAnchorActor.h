#pragma once

#include "GameFramework/Actor.h"
#include "MikanAnchorActor.generated.h"

UCLASS(BlueprintType)
class MIKANXR_API AMikanAnchorActor : public AActor
{
	GENERATED_BODY()

public:
	AMikanAnchorActor(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UMikanAnchorComponent* AnchorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UTextRenderComponent* LabelComponent;

	UFUNCTION(BlueprintPure)
	class AMikanScene* GetParentScene() const;

	UFUNCTION(BlueprintPure)
	int32 GetAnchorId() const;

	UFUNCTION(BlueprintPure)
	const FString& GetAnchorName() const;

	void UnbindAnchorId();
	void ApplyAnchorInfo(const struct MikanSpatialAnchorInfo& InAnchorInfo);
	void UpdateLabelText();
};
