#pragma once

#include "GameFramework/Actor.h"
#include "MikanStencilActor.h"
#include "MikanStencilTypes.h"
#include "MikanBoxStencilActor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MIKANXR_API AMikanBoxStencilActor : public AMikanStencilActor
{
	GENERATED_BODY()

public:
	AMikanBoxStencilActor(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	static AMikanBoxStencilActor* SpawnStencil(
		class AMikanScene* OwnerScene,
		const MikanStencilBoxInfo& BoxStencilInfo);
	void ApplyBoxStencilInfo(const struct MikanStencilBoxInfo& InStencilInfo);

	UFUNCTION(BlueprintPure)
	FVector GetExtents() const { return Extents; }

protected:
	FVector Extents;
};
