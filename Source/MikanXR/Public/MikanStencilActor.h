#pragma once

#include "GameFramework/Actor.h"
#include "MikanStencilActor.generated.h"

UCLASS(BlueprintType)
class MIKANXR_API AMikanStencilActor : public AActor
{
	GENERATED_BODY()

public:
	AMikanStencilActor(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UMikanStencilComponent* StencilComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UTextRenderComponent* LabelComponent;

	UFUNCTION(BlueprintPure)
	class AMikanScene* GetParentScene() const;

	UFUNCTION(BlueprintPure)
	int32 GetStencilId() const;

	void ApplyStencilName(const FString& InStencilName);

protected:
	void UpdateLabelText();
};
