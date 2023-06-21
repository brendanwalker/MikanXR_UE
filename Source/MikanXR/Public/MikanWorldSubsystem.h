#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "MikanClientTypes.h"
#include "MikanWorldSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMikanWorldEvent);

UCLASS(HideCategories = (Navigation, Rendering, Physics, Collision, Cooking, Input, Actor))
class MIKANXR_API UMikanWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UMikanWorldSubsystem();

	static UMikanWorldSubsystem* GetInstance(class UWorld* CurrentWorld);

	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RegisterAnchor(class UMikanAnchorComponent* InAnchor);
	void UnregisterAnchor(class UMikanAnchorComponent* InAnchor);

	void BindMikanCamera(class AMikanCamera* InCamera);
	void UnbindCamera(class AMikanCamera* InCamera);

	void HandleMikanEvent(MikanEvent* event);

	void HandleMikanConnected();
	UPROPERTY(BlueprintAssignable)
	FMikanWorldEvent OnMikanConnected;

	void HandleMikanDisconnected();
	UPROPERTY(BlueprintAssignable)
	FMikanWorldEvent OnMikanDisconnected;


protected:
	void UpdateAnchorPose(const MikanAnchorPoseUpdateEvent& AnchorPoseEvent);
	void ProcessNewVideoSourceFrame(const MikanVideoSourceNewFrameEvent& newFrameEvent);
	void FreeFrameBuffer();
	void ReallocateRenderBuffers();
	void UpdateCameraProperties();
	void HandleVideoSourceAttachmentChanged();
	void UpdateCameraAttachment();

	UPROPERTY(Transient)
	TArray<class UMikanAnchorComponent*> Anchors;

	UPROPERTY(Transient)
	class AMikanCamera* MikanCamera;

	MikanSpatialAnchorID CameraParentAnchorId= INVALID_MIKAN_ID;
	float MikanCameraScale= 1.f;
	MikanClientInfo ClientInfo;
	MikanRenderTargetMemory RenderTargetMemory;
	uint64 LastReceivedVideoSourceFrame;
};
