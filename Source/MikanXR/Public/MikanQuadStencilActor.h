#pragma once

#include "GameFramework/Actor.h"
#include "MikanStencilActor.h"
#include "MikanStencilTypes.h"
#include "MikanQuadStencilActor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MIKANXR_API AMikanQuadStencilActor : public AMikanStencilActor
{
	GENERATED_BODY()

public:
	AMikanQuadStencilActor(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	static AMikanQuadStencilActor* SpawnStencil(
		class AMikanScene* OwnerScene,
		const MikanStencilQuadInfo& QuadStencilInfo);
	void ApplyQuadStencilInfo(const struct MikanStencilQuadInfo& InStencilInfo);

	UFUNCTION(BlueprintPure)
	FVector2D GetQuadSize() const { return QuadSize; }

private:
	FVector2D QuadSize;
};
