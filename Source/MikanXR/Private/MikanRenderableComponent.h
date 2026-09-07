#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MikanRenderableComponent.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UMikanRenderableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMikanRenderableComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure)
	bool OverlapsCone(const FVector& Origin, const FVector& Direction, float HalfAngleDegrees) const;

protected:
	UPROPERTY(Transient)
	class AMikanClient* OwnerMikanClient= nullptr;
};
