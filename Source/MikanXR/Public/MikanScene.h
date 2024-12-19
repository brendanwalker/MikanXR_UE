#pragma once

#include "MikanAPITypes.h"
#include "GameFramework/Actor.h"
#include "MikanScene.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MIKANXR_API AMikanScene : public AActor
{
	GENERATED_BODY()

public:
	AMikanScene(const FObjectInitializer& ObjectInitializer);

	virtual void PostRegisterAllComponents() override;
	virtual void Destroyed() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class USceneComponent* SceneOrigin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<class AMikanCamera> CameraClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* ModelStencilMaterial;

	UFUNCTION(BlueprintPure)
	class AMikanCamera* GetMikanCamera() const { return SceneCamera; }

	class IMikanAPI* GetMikanAPI() const { return MikanAPI; }

	class AMikanStencilActor* GetMikanStencilById(MikanStencilID StencilID);

	// Scene Events
	void HandleSceneActivated();
	void HandleSceneDeactivated();

	// App Connection Events
	UFUNCTION(CallInEditor)
	void HandleMikanConnected();

	// Spatial Anchor Events
	UFUNCTION(CallInEditor)
	void HandleAnchorListChanged();
	UFUNCTION(CallInEditor)
	void HandleAnchorNameChanged(int32 AnchorID, const FString& AnchorName);
	UFUNCTION(CallInEditor)
	void HandleAnchorPoseChanged(int32 AnchorID, const FTransform& SceneTransform);

	// Stencil Events
	UFUNCTION()
	void HandleQuadStencilListChanged();
	UFUNCTION()
	void HandleBoxStencilListChanged();
	UFUNCTION()
	void HandleModelStencilListChanged();
	UFUNCTION()
	void HandleStencilNameChanged(int32 StencilID, const FString& StencilName);
	UFUNCTION()
	void HandleStencilPoseChanged(int32 StencilID, const FTransform& SceneTransform);

	// Script Message Events
	UFUNCTION()
	void HandleScriptMessage(const FString& Message);
	UFUNCTION(BlueprintImplementableEvent)
	void OnMikanScriptMessage(const FString& Message);

protected:
	void GatherSceneAnchorList(TArray<class AMikanAnchorActor *>& OutAnchorActors, int32 AnchorIdFilter=-1);
	void ApplyAnchorInfo(const struct MikanSpatialAnchorInfo& AnchorInfo);
	void SpawnSceneCamera();
	void DespawnSceneCamera();
	void DespawnAllQuadStencils();
	void DespawnAllBoxStencils();
	void DespawnAllModelStencils();

	UPROPERTY(Transient)
	class AMikanCamera* SceneCamera= nullptr;

	// A table of child Quad Stencil Actors (exist only at during Play)
	UPROPERTY(Transient)
	TMap<int32, class AMikanQuadStencilActor*> QuadStencils;

	// A table of child Box Stencil Actors (exist only at during Play)
	UPROPERTY(Transient)
	TMap<int32, class AMikanBoxStencilActor*> BoxStencils;

	// A table of child Model Stencil Actors (exist only at during Play)
	UPROPERTY(Transient)
	TMap<int32, class AMikanModelStencilActor*> ModelStencils;

	bool bIsScenePlaying= false;

	class IMikanAPI* MikanAPI= nullptr;
};
