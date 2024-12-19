// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "MikanCoreTypes.h"
#include "MikanVideoSourceTypes.h"
#include "MikanCaptureComponent.generated.h"

UCLASS(hidecategories=(Collision, Object, Physics, SceneComponent), ClassGroup=Rendering, editinlinenew, meta=(BlueprintSpawnableComponent))
class MIKANXR_API UMikanCaptureComponent : public USceneCaptureComponent2D
{
	GENERATED_BODY()

public:
	UMikanCaptureComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetRenderTargetDesc(const MikanRenderTargetDescriptor& InRTDdesc);
	void SetVideoSourceIntrinsics(const MikanMonoIntrinsics& InIntrinsics);

	void CaptureFrame(uint64 NewFrameIndex);

	// Mikan UE4 Events
	UFUNCTION()
	void HandleMikanRenderableRegistered(class UMikanRenderableComponent* Renderable);
	UFUNCTION()
	void HandleMikanRenderableUnregistered(class UMikanRenderableComponent* Renderable);

protected:
	class IMikanAPI* MikanAPI= nullptr;
	MikanRenderTargetDescriptor RenderTargetDesc;
	MikanMonoIntrinsics VideoSourceIntrinsics;
	float NearClippingPlaneUU;
	float FarClippingPlaneUU;
};
