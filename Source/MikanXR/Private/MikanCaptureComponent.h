// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "RHIFwd.h"
#include "PixelFormat.h"
#include "MikanCoreTypes.h"
#include "MikanVideoSourceTypes.h"
#include "MikanCaptureComponent.generated.h"

class FTextureRenderTargetResource;

// Which Mikan buffer this capture component feeds. Determines both the per-actor show-only tag
// it filters on and the shared-texture write request it issues on publish.
UENUM()
enum class EMikanCaptureKind : uint8
{
	Color,
	Depth,
	// Shadowed pass (A): white catcher lit with shadow casters present. Includes untagged
	// renderables (e.g. the character) so they cast shadows onto the catcher.
	Shadow,
	// Reference pass (B): the same white catcher with NO casters. Excludes untagged renderables.
	// Divided into Shadow (A/B) to isolate the per-channel shadow factor.
	ShadowReference,
};

UCLASS(hidecategories=(Collision, Object, Physics, SceneComponent), ClassGroup=Rendering, editinlinenew, meta=(BlueprintSpawnableComponent))
class UMikanCaptureComponent : public USceneCaptureComponent2D
{
	GENERATED_BODY()

public:
	static FName MikanRenderColor;
	static FName MikanRenderDepth;
	static FName MikanRenderShadow;

	UMikanCaptureComponent(const FObjectInitializer& ObjectInitializer);

	void SetCaptureKind(EMikanCaptureKind InCaptureKind) { CaptureKind = InCaptureKind; }
	EMikanCaptureKind GetCaptureKind() const { return CaptureKind; }

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetRenderTargetDesc(const MikanRenderTargetDescriptor& InRTDdesc);
	void SetCustomProjectionMatrix(const FMatrix& InProjectionMatrix);
	void SetDepthClippingPlanes(float InNearClippingPlaneUU, float InFarClippingPlaneUU);

	// Kicks off the scene capture for this frame's render target. The actual GPU work is
	// enqueued on the render thread and is not complete when this returns.
	void CaptureFrame();

	// Captures in flight at once. Each holds a staging texture until its publish, so this is
	// also the depth of the staging ring.
	static constexpr int32 kStagingRingDepth = 3;

	// A copy of a render target into one of this component's shared staging textures, described
	// on the game thread and executed on the render thread behind the captures it reads from.
	struct FStagingCopy
	{
		UMikanCaptureComponent* Component = nullptr;
		FTextureRenderTargetResource* Resource = nullptr;
		int32 SlotIndex = 0;
		int32 Width = 0;
		int32 Height = 0;
		EPixelFormat Format = PF_Unknown;
	};

	// Describes the copy of SourceTarget into the staging slot. Returns false when the target has
	// nothing to copy from. SourceTarget is normally this component's own capture target; the
	// shadow pass passes the A/B divide result instead.
	bool PrepareStagingCopy(class UTextureRenderTarget2D* SourceTarget, int32 SlotIndex, FStagingCopy& OutCopy);

	// Executes a prepared copy, (re)creating the slot's staging texture when its size or format changed
	static void ExecuteStagingCopy_RenderThread(FRHICommandListImmediate& RHICmdList, const FStagingCopy& Copy);

	// Hands a staging slot to Mikan as this component's buffer kind. Call only after a GPU fence
	// written behind that slot's copy has signalled, so Mikan reads a settled texture.
	void PublishSharedStaging(int32 CameraId, int32 SlotIndex);

	// Mikan UE4 Events
	UFUNCTION()
	void HandleMikanRenderableRegistered(class UMikanRenderableComponent* Renderable);
	UFUNCTION()
	void HandleMikanRenderableUnregistered(class UMikanRenderableComponent* Renderable);

protected:
	class AMikanClient* GetOwnerMikanClient() const;

protected:
	EMikanCaptureKind CaptureKind= EMikanCaptureKind::Color;
	class IMikanAPI* MikanAPI= nullptr;
	MikanRenderTargetDescriptor RenderTargetDesc;
	MikanMonoIntrinsics VideoSourceIntrinsics;
	float NearClippingPlaneUU;
	float FarClippingPlaneUU;

	// Dedicated staging textures that Mikan wraps and copies to the Spout shared texture. We copy
	// the scene-capture render target into these instead of handing Mikan the live render target
	// directly, so Unreal's renderer never re-renders into the resource Mikan has wrapped (which
	// caused D3D12 resource-state conflicts and flicker). Created with the Shared flag and left in
	// SRVMask so the rest state matches Mikan's GENERIC_READ wrap. A ring rather than one texture
	// so the next capture's copy can land while an earlier capture still waits on its fence.
	// Written on the render thread; the game thread reads a slot only after its fence signalled.
	struct FStagingSlot
	{
		FTextureRHIRef Texture;
		int32 Width = 0;
		int32 Height = 0;
		EPixelFormat Format = PF_Unknown;
	};
	FStagingSlot StagingSlots[kStagingRingDepth];
};
