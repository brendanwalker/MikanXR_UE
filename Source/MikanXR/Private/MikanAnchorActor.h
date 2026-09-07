#pragma once

#include "GameFramework/Actor.h"
#include "MikanTransformActor.h"
#include "MikanDataStore.h"
#include "MikanAnchorActor.generated.h"

UCLASS()
class UMikanAnchorData : public UMikanTransformData
{
	GENERATED_BODY()

public:
	UMikanAnchorData()= default;
};

UCLASS(BlueprintType)
class AMikanAnchorActor : public AMikanTransformActor
{
	GENERATED_BODY()

public:
	AMikanAnchorActor(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
};
