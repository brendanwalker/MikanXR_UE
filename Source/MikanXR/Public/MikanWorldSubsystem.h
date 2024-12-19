#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MikanAPI.h"
#include "MikanWorldSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMikanSimpleEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMikanNameUpdateEvent, int, ObjectId, const FString&, ObjectName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMikanPoseUpdateEvent, int, ObjectId, const FTransform&, SceneTransform);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMikanVRDevicePoseUpdateEvent, int, DeviceId, int64, FrameIndex, const FTransform&, SceneTransform);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMikanNewFrameEvent, int64, FrameIndex, const FTransform&, SceneTransform);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMikanScriptMessageEvent, const FString&, ScriptMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMikanActiveSceneChangeEvent, class AMikanScene*, OldScene, class AMikanScene*, NewScene);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMikanRenderableEvent, class UMikanRenderableComponent*, Renderable);

UCLASS(HideCategories = (Navigation, Rendering, Physics, Collision, Cooking, Input, Actor))
class MIKANXR_API UMikanWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UMikanWorldSubsystem();

	// UWorldSubsystem API
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// UMikanWorldSubsystem API
	static UMikanWorldSubsystem* GetInstance(class UWorld* CurrentWorld);

	inline class IMikanAPI* GetMikanAPI() { return MikanAPI; }
	inline const MikanClientInfo& GetClientInfo() const { return ClientInfo; }

	bool RegisterMikanScene(class AMikanScene* MikanScene);
	void UnregisterMikanScene(class AMikanScene* MikanScene);
	UFUNCTION(BlueprintCallable)
	void SetActiveMikanScene(class AMikanScene* DesiredScene);
	UFUNCTION(BlueprintPure)
	inline class AMikanScene* GetActiveMikanScene() const { return ActiveMikanScene; }

	bool RegisterMikanRenderable(class UMikanRenderableComponent* Renderable);
	void UnregisterMikanRenderable(class UMikanRenderableComponent* Renderable);
	UFUNCTION(BlueprintPure)
	inline TArray<class UMikanRenderableComponent*>& GetRegisteredMikanRenderables()
	{ return RegisteredRenderables; }

	void HandleMikanEvent(MikanEventPtr mikanEvent);

	// Scene Events
	UPROPERTY(BlueprintAssignable)
	FMikanActiveSceneChangeEvent OnActiveSceneChanged;

	// Renderable Events
	UPROPERTY(BlueprintAssignable)
	FMikanRenderableEvent OnRenderableRegistered;
	UPROPERTY(BlueprintAssignable)
	FMikanRenderableEvent OnRenderableUnregistered;

	// App Connection Events
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnMikanConnected;
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnMikanDisconnected;

	// Video Source Events
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnVideoSourceOpened;
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnVideoSourceClosed;
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnVideoSourceModeChanged;
	UPROPERTY(BlueprintAssignable)
	FMikanNewFrameEvent OnNewFrameEvent;
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnVideoSourceIntrinsicsChanged;
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnVideoSourceAttachmentChanged;

	// VR Device Events
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnVRDeviceListChanged;
	UPROPERTY(BlueprintAssignable)
	FMikanVRDevicePoseUpdateEvent OnVRDevicePoseChanged;

	// Spatial Anchor Events
	UPROPERTY(BlueprintAssignable)
	FMikanNameUpdateEvent OnAnchorNameChanged;
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnAnchorListChanged;
	UPROPERTY(BlueprintAssignable)
	FMikanPoseUpdateEvent OnAnchorPoseChanged;

	// Stencil Events
	UPROPERTY(BlueprintAssignable)
	FMikanNameUpdateEvent OnStencilNameChanged;
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnQuadStencilListChanged;
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnBoxStencilListChanged;
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnModelStencilListChanged;
	UPROPERTY(BlueprintAssignable)
	FMikanPoseUpdateEvent OnStencilPoseChanged;

	// Script Message Events
	UPROPERTY(BlueprintAssignable)
	FMikanScriptMessageEvent OnScriptMessage;

protected:
	// App Connection Events
	void HandleMikanConnected();
	void HandleMikanDisconnected();

	// Video Source Events
	void HandleVideoSourceOpened();
	void HandleVideoSourceClosed();
	void HandleVideoSourceModeChanged();
	void HandleVideoSourceIntrinsicsChanged();
	void HandleVideoSourceAttachmentChanged();
	void HandleNewVideoSourceFrame(const struct MikanVideoSourceNewFrameEvent& newFrameEvent);

	// VR Device Events
	void HandleVRDeviceListChanged();
	void HandleVRDevicePoseChanged(const struct MikanVRDevicePoseUpdateEvent& DevicePoseEvent);

	// Spatial Anchor Events
	void HandleAnchorListChanged();
	void HandleAnchorNameChanged(const struct MikanAnchorNameUpdateEvent& AnchorNameEvent);
	void HandleAnchorPoseChanged(const struct MikanAnchorPoseUpdateEvent& AnchorPoseEvent);

	// Stencil Events
	void HandleStencilNameChanged(const struct MikanStencilNameUpdateEvent& StencilNameEvent);
	void HandleQuadStencilListChanged();
	void HandleBoxStencilListChanged();
	void HandleModelStencilListChanged();
	void HandleStencilPoseChanged(const struct MikanStencilPoseUpdateEvent& StencilPoseEvent);

	// Script Message Events
	void HandleScriptMessage(const struct MikanScriptMessagePostedEvent& ScriptMessageEvent);

	UPROPERTY(Transient)
	TArray<class AMikanScene*> RegisteredScenes;
	UPROPERTY(Transient)
	AMikanScene* ActiveMikanScene= nullptr;

	UPROPERTY(Transient)
	TArray<class UMikanRenderableComponent*> RegisteredRenderables;

	// MikanXR API
	class IMikanAPI* MikanAPI= nullptr;
	MikanClientInfo ClientInfo;
};
