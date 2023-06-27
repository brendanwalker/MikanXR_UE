#pragma once

#include "GameFramework/Actor.h"
#include "MikanClientTypes.h"
#include "MikanScene.generated.h"

struct FMikanAnchorInfo
{
	MikanSpatialAnchorID AnchorID= INVALID_MIKAN_ID;
	FTransform MikanSpaceTransform; 
	FString AnchorName;
};

UCLASS(Blueprintable)
class MIKANXR_API AMikanScene : public AActor
{
	GENERATED_BODY()

public:
	AMikanScene(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class USceneComponent* SceneOrigin;

	// The name of the anchor to make everything in the scene relative to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	FString SceneOriginAnchorName;

	UFUNCTION(BlueprintPure)
	class AMikanCamera* GetMikanCamera() const { return SceneCamera; }

	UFUNCTION(BlueprintPure)
	const FTransform& GetMikanToSceneTransform() const { return MikanToSceneTransform; }

	FMikanAnchorInfo* GetMikanAnchorInfoById(MikanSpatialAnchorID AnchorId);
	FMikanAnchorInfo* GetMikanAnchorInfoByName(const FString& AnchorName);

	void HandleMikanConnected();
	void HandleMikanDisconnected();
	void HandleAnchorListChanged();
	void HandleAnchorPoseChanged(const MikanAnchorPoseUpdateEvent& AnchorPoseEvent);
	void HandleCameraIntrinsicsChanged();
	void HandleCameraAttachmentChanged();
	void HandleNewVideoSourceFrame(const MikanVideoSourceNewFrameEvent& newFrameEvent);

protected:
	void RecomputeMikanToSceneTransform();
	void RebuildSceneAnchorList();
	void BindSceneCamera();

	// Transform used to convert Mikan space transforms to Scene space transforms.
	FTransform MikanToSceneTransform;

	// Uniform scale appplied to the mikan transform
	float MikanSceneScale = 1.f;

	// The table of anchors fetched from Mikan
	TMap<MikanSpatialAnchorID, FMikanAnchorInfo> MikanAnchorMap;

	UPROPERTY(Transient)
	class AMikanCamera* SceneCamera= nullptr;

	// A list of child AnchorComponents bound to corresponding FMikanAnchorInfo by name
	UPROPERTY(Transient)
	TArray<class UMikanAnchorComponent*> SceneAnchors;
};
