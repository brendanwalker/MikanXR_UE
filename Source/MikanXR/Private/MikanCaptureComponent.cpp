#include "MikanCaptureComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "MikanWorldSubsystem.h"
#include "MikanClient_CAPI.h"

UMikanCaptureComponent::UMikanCaptureComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	bWantsInitializeComponent = true;
	bCaptureEveryFrame= false; // Wait for CaptureFrame call from Mikan

	this->SetRelativeScale3D(FVector(1.f));
	this->SetRelativeLocation(FVector::ZeroVector);
}

void UMikanCaptureComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UMikanCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UMikanCaptureComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UMikanCaptureComponent::SetRenderTargetDesc(const MikanRenderTargetDescriptor& InRTDdesc)
{
	RTDdesc= InRTDdesc;
}

void UMikanCaptureComponent::CaptureFrame(uint64 NewFrameIndex)
{
	if (TextureTarget != nullptr)
	{
		CaptureScene();
		LastRenderedFrameIndex= NewFrameIndex;

		MikanClientGraphicsApi api= RTDdesc.graphicsAPI;
		if (api == MikanClientGraphicsApi_Direct3D9 ||
			api == MikanClientGraphicsApi_Direct3D11 ||
			api == MikanClientGraphicsApi_Direct3D12 ||
			api == MikanClientGraphicsApi_OpenGL)
		{
			UTextureRenderTarget2D* RenderTarget2D= TextureTarget;

			ENQUEUE_RENDER_COMMAND(void)(
				[RenderTarget2D, NewFrameIndex](FRHICommandListImmediate& RHICmdList) {
					FTextureRenderTargetResource* TextureResource = RenderTarget2D->GetRenderTargetResource();
					if (TextureResource != nullptr)
					{
						FRHITexture2D* TextureRHI = TextureResource->GetTexture2DRHI();
						if (TextureRHI != nullptr)
						{
							void* NativeTexturePtr = TextureRHI->GetNativeResource();
							if (NativeTexturePtr != nullptr)
							{
								Mikan_PublishRenderTargetTexture(NativeTexturePtr, NewFrameIndex);
								//g_pImmediateContext->CopyResource(targetTex, baseTexture);
								//g_pImmediateContext->Flush();
							}
						}
					}

				});
		}
		else
		{
			//TODO: Slow Async Readback 
		}
	}
}
