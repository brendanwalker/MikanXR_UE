#pragma once

#include "GameFramework/Actor.h"
#include "MikanStencilActor.h"
#include "MikanStencilTypes.h"
#include "ProceduralMeshComponent.h"
#include "MikanModelStencilActor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MIKANXR_API AMikanModelStencilActor : public AMikanStencilActor
{
	GENERATED_BODY()

public:
	AMikanModelStencilActor(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UProceduralMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* Material;

	static AMikanModelStencilActor* SpawnStencil(
		class AMikanScene* OwnerScene,
		const MikanStencilModelInfo& modelStencilInfo);
	void ApplyModelStencilInfo(const struct MikanStencilModelInfo& InStencilInfo);
	void ApplyModelRenderGeometry(const MikanStencilModelRenderGeometry& InModelInfo);
	void RefetchModelRenderGeometry();
};
