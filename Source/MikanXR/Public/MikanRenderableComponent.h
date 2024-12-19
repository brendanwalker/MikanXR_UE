#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MikanRenderableComponent.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class MIKANXR_API UMikanRenderableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMikanRenderableComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
