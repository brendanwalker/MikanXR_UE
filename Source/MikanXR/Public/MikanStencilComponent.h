#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "MikanStencilTypes.h"
#include "MikanStencilComponent.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class MIKANXR_API UMikanStencilComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UMikanStencilComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 StencilId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString StencilName;

	UFUNCTION(BlueprintPure)
	class AMikanScene* GetParentScene() const;

	void ApplyStencilInfo(
		MikanStencilID StencilId, 
		const std::string& StencilName, 
		const MikanTransform& RelativeTransform);
	void ApplyStencilName(const FString& InStencilName);
	void ApplyStencilTransform(const MikanTransform& RelativeTransform);
};
