#pragma once

#include "GameFramework/Actor.h"
#include "MikanAPITypes.h"
#include "MikanClientTypes.h"
#include "MikanVideoSourceTypes.h"
#include "MikanCamera.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MIKANXR_API AMikanCamera : public AActor
{
	GENERATED_BODY()

public:
	AMikanCamera(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds ) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* CameraRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UMikanCaptureComponent* ColorCaptureComponent;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Components")
	class UTextureRenderTarget2D* ColorRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UMikanCaptureComponent* DepthCaptureComponent;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Components")
	class UTextureRenderTarget2D* DepthRenderTarget;

	UFUNCTION(BlueprintPure)
	class AMikanScene* GetParentScene() const;

	// Mikan API Events
	UFUNCTION()
	void HandleMikanConnected();
	UFUNCTION()
	void HandleMikanDisconnected();
	UFUNCTION()
	void HandleVideoSourceOpened();
	UFUNCTION()
	void HandleVideoSourceModeChanged();
	UFUNCTION()
	void HandleVideoSourceIntrinsicsChanged();
	UFUNCTION()
	void HandleNewVideoSourceFrame(int64 FrameIndex, const FTransform& SceneTransform);

protected:
	void ReallocateRenderBuffers();
	void FreeRenderBuffers();
	void RecreateRenderTargets(const MikanRenderTargetDescriptor& InRTDdesc);
	void DisposeRenderTargets();
	
protected:
	class IMikanAPI* MikanAPI= nullptr;
	MikanClientInfo ClientInfo;
	MikanMonoIntrinsics MonoIntrinsics;
	MikanRenderTargetDescriptor RenderTargetDesc;
	uint64 LastRenderedFrameIndex = 0;
};
