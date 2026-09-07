#include "MikanCaptureComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/Material.h"
#include "MikanCameraRequests.h"
#include "MikanCameraActor.h"
#include "MikanClient.h"
#include "MikanRenderTargetRequests.h"
#include "MikanRenderableComponent.h"
#include "MikanTransformActor.h"
#include "MikanAPI.h"
#include "MikanMath.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "RHIResources.h"

FName UMikanCaptureComponent::MikanRenderColor = FName(TEXT("MikanRenderColor"));
FName UMikanCaptureComponent::MikanRenderDepth = FName(TEXT("MikanRenderDepth"));
FName UMikanCaptureComponent::MikanRenderShadow = FName(TEXT("MikanRenderShadow"));

UMikanCaptureComponent::UMikanCaptureComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	bWantsInitializeComponent = true;
	bCaptureEveryFrame= false; // Wait for CaptureFrame call from Mikan
	bCaptureOnMovement= false; // Wait for CaptureFrame call from Mikan

	// Rendering defaults for transparent background and opt-in actors
	PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
	PostProcessBlendWeight = 0.0f;
	ShowFlags.SetAtmosphere(false);
	ShowFlags.SetFog(false);
	bConsiderUnrenderedOpaquePixelAsFullyTranslucent = true;
	NearClippingPlaneUU = 1;
	FarClippingPlaneUU = 1000;

	this->SetRelativeScale3D(FVector(1.f));
	this->SetRelativeLocation(FVector::ZeroVector);
}

void UMikanCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	AMikanClient* MikanClient = GetOwnerMikanClient();
	if (MikanClient)
	{
		MikanAPI = MikanClient->GetMikanAPI();

		MikanClient->OnRenderableRegistered.AddDynamic(this, &UMikanCaptureComponent::HandleMikanRenderableRegistered);
		MikanClient->OnRenderableUnregistered.AddDynamic(this, &UMikanCaptureComponent::HandleMikanRenderableUnregistered);

		// Register all existing renderables
		const auto& ExistingRenderables= MikanClient->GetRegisteredMikanRenderables();
		for (UMikanRenderableComponent* Renderable : ExistingRenderables)
		{
			HandleMikanRenderableRegistered(Renderable);
		}	
	}
}

void UMikanCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AMikanClient* MikanClient = GetOwnerMikanClient();
	if (MikanClient)
	{
		MikanClient->OnRenderableRegistered.RemoveDynamic(this, &UMikanCaptureComponent::HandleMikanRenderableRegistered);
		MikanClient->OnRenderableUnregistered.RemoveDynamic(this, &UMikanCaptureComponent::HandleMikanRenderableUnregistered);
	}

	// Release the staging texture. We flush first so no in-flight render command still references it.
	if (SharedStagingTexture.IsValid())
	{
		FlushRenderingCommands();
		SharedStagingTexture.SafeRelease();
		SharedStagingWidth = 0;
		SharedStagingHeight = 0;
		SharedStagingFormat = PF_Unknown;
	}

	Super::EndPlay(EndPlayReason);
}

void UMikanCaptureComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UMikanCaptureComponent::SetRenderTargetDesc(const MikanRenderTargetDescriptor& InRTDdesc)
{
	RenderTargetDesc= InRTDdesc;
}

void UMikanCaptureComponent::SetCustomProjectionMatrix(const FMatrix& InProjectionMatrix)
{
	CustomProjectionMatrix = InProjectionMatrix;
	bUseCustomProjectionMatrix = true;
}

void UMikanCaptureComponent::SetDepthClippingPlanes(
	float InNearClippingPlaneUU,
	float InFarClippingPlaneUU)
{
	NearClippingPlaneUU = InNearClippingPlaneUU;
	FarClippingPlaneUU = InFarClippingPlaneUU;
}

void UMikanCaptureComponent::CaptureFrame()
{
	if (TextureTarget != nullptr)
	{
		// Enqueues the scene capture on the render thread. The GPU work is NOT finished when
		// this returns, so we don't read the render target here - see PublishCapturedFrame().
		CaptureScene();
	}
}

void UMikanCaptureComponent::PublishCapturedFrame(int32 CameraId)
{
	PublishRenderTarget(CameraId, TextureTarget);
}

void UMikanCaptureComponent::PublishRenderTarget(int32 CameraId, UTextureRenderTarget2D* SourceTarget)
{
	if (SourceTarget == nullptr)
	{
		return;
	}

	MikanClientGraphicsApi api = RenderTargetDesc.graphicsAPI;
	if (api != MikanClientGraphicsApi_Direct3D9 &&
		api != MikanClientGraphicsApi_Direct3D11 &&
		api != MikanClientGraphicsApi_Direct3D12 &&
		api != MikanClientGraphicsApi_OpenGL)
	{
		return;
	}

	// Copy the captured render target into our dedicated staging texture and get its native
	// handle. We hand Mikan this staging texture rather than the live render target so Unreal
	// never re-renders into the resource Mikan has wrapped for Spout - that dual ownership of one
	// resource's D3D12 state was the source of the "before state does not match" barrier errors
	// and the residual flicker. UpdateSharedStagingTexture() also blocks until the GPU copy is
	// complete, so Mikan reads a fully rendered, settled texture.
	void* NativeTexturePtr = UpdateSharedStagingTexture(SourceTarget);
	if (NativeTexturePtr == nullptr)
	{
		return;
	}

	switch (CaptureKind)
	{
	case EMikanCaptureKind::Color:
		{
			WriteCameraColorRenderTargetTexture writeRequest;
			writeRequest.camera_id = CameraId;
			writeRequest.api_color_texture_ptr = NativeTexturePtr;

			MikanAPI->sendRequest(writeRequest);
		}
		break;
	case EMikanCaptureKind::Depth:
		{
			WriteCameraDepthRenderTargetTexture writeRequest;
			writeRequest.camera_id = CameraId;
			writeRequest.api_depth_texture_ptr = NativeTexturePtr;
			writeRequest.z_near = NearClippingPlaneUU;
			writeRequest.z_far = FarClippingPlaneUU;

			MikanAPI->sendRequest(writeRequest);
		}
		break;
	case EMikanCaptureKind::Shadow:
		{
			WriteCameraShadowRenderTargetTexture writeRequest;
			writeRequest.camera_id = CameraId;
			writeRequest.api_shadow_texture_ptr = NativeTexturePtr;

			MikanAPI->sendRequest(writeRequest);
		}
		break;
	}
}

void* UMikanCaptureComponent::UpdateSharedStagingTexture(UTextureRenderTarget2D* SourceTarget)
{
	if (SourceTarget == nullptr)
	{
		return nullptr;
	}

	FTextureRenderTargetResource* RTResource = SourceTarget->GameThread_GetRenderTargetResource();
	if (RTResource == nullptr)
	{
		return nullptr;
	}

	const int32 Width = SourceTarget->SizeX;
	const int32 Height = SourceTarget->SizeY;
	const EPixelFormat Format = SourceTarget->GetFormat();
	if (Width <= 0 || Height <= 0 || Format == PF_Unknown)
	{
		return nullptr;
	}

	ENQUEUE_RENDER_COMMAND(MikanCopyToSharedStaging)(
		[this, RTResource, Width, Height, Format](FRHICommandListImmediate& RHICmdList)
		{
			// (Re)create the staging texture if it is missing or the size/format changed.
			if (!SharedStagingTexture.IsValid()
				|| SharedStagingWidth != Width
				|| SharedStagingHeight != Height
				|| SharedStagingFormat != Format)
			{
				FRHITextureCreateDesc Desc =
					FRHITextureCreateDesc::Create2D(TEXT("MikanSharedStaging"), Width, Height, Format)
						.SetFlags(ETextureCreateFlags::ShaderResource
								  | ETextureCreateFlags::RenderTargetable
								  | ETextureCreateFlags::Shared);

				SharedStagingTexture = RHICmdList.CreateTexture(Desc);
				SharedStagingWidth = Width;
				SharedStagingHeight = Height;
				SharedStagingFormat = Format;
			}

			FRHITexture* Src = RTResource->GetRenderTargetTexture();
			FRHITexture* Dst = SharedStagingTexture.GetReference();
			if (Src == nullptr || Dst == nullptr)
			{
				return;
			}

			RHICmdList.Transition(FRHITransitionInfo(Src, ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.Transition(FRHITransitionInfo(Dst, ERHIAccess::Unknown, ERHIAccess::CopyDest));

			RHICmdList.CopyTexture(Src, Dst, FRHICopyTextureInfo());

			// Leave the staging texture in SRVMask so its resource state matches the GENERIC_READ
			// state Mikan wraps it with - this is what stops 11on12 and Unreal's state tracker from
			// disagreeing about the resource's state.
			RHICmdList.Transition(FRHITransitionInfo(Dst, ERHIAccess::CopyDest, ERHIAccess::SRVMask));

			// Block until the GPU has finished the copy so Mikan reads a complete, settled texture.
			RHICmdList.SubmitCommandsAndFlushGPU();
		});

	// Wait for the render thread (and the GPU flush above) to complete before reading the staging
	// texture's native handle back on the game thread.
	FlushRenderingCommands();

	if (!SharedStagingTexture.IsValid())
	{
		return nullptr;
	}

	return SharedStagingTexture->GetNativeResource();
}

AMikanClient* UMikanCaptureComponent::GetOwnerMikanClient() const
{
	if (auto* OwnerFrameCaptureActor = Cast<AMikanFrameCaptureActor>(GetOwner()))
	{
		if (AMikanCameraActor* OwnerCameraActor = OwnerFrameCaptureActor->GetOwnerCameraActor())
		{
			return OwnerCameraActor->GetOwnerMikanClient();
		}
	}

	return nullptr;
}

// Mikan UE4 Events
void UMikanCaptureComponent::HandleMikanRenderableRegistered(UMikanRenderableComponent* Renderable)
{
	if (Renderable)
	{
		AActor* RenderableOwner= Renderable->GetOwner();
		TArray<UActorComponent*> ColorRenderComponents =
			RenderableOwner->GetComponentsByTag(UPrimitiveComponent::StaticClass(), MikanRenderColor);
		TArray<UActorComponent*> DepthRenderComponents =
			RenderableOwner->GetComponentsByTag(UPrimitiveComponent::StaticClass(), MikanRenderDepth);
		TArray<UActorComponent*> ShadowRenderComponents =
			RenderableOwner->GetComponentsByTag(UPrimitiveComponent::StaticClass(), MikanRenderShadow);

		// If the actor opts into the Mikan per-buffer tag system, only the components tagged for
		// this capture's buffer are shown. Otherwise the whole actor is shown in every capture
		// (e.g. a virtual character that should appear in color and cast shadows into the shadow pass).
		const bool bIsShadowKind=
			CaptureKind == EMikanCaptureKind::Shadow || CaptureKind == EMikanCaptureKind::ShadowReference;

		if (ColorRenderComponents.Num() > 0 || DepthRenderComponents.Num() > 0 || ShadowRenderComponents.Num() > 0)
		{
			const TArray<UActorComponent*>& KindComponents=
				(CaptureKind == EMikanCaptureKind::Depth) 
				? DepthRenderComponents
				: (bIsShadowKind ? ShadowRenderComponents : ColorRenderComponents);

			for (UActorComponent* Component : KindComponents)
			{
				ShowOnlyComponents.Add(CastChecked<UPrimitiveComponent>(Component));
			}
		}
		// Untagged renderable (e.g. a virtual character). It's a shadow caster, not a catcher, so
		// it belongs in every pass EXCEPT the reference pass (B), whose whole purpose is to capture
		// the catcher with no casters present.
		else if (CaptureKind != EMikanCaptureKind::ShadowReference)
		{
			ShowOnlyActors.AddUnique(Renderable->GetOwner());
		}
	}
}

void UMikanCaptureComponent::HandleMikanRenderableUnregistered(UMikanRenderableComponent* Renderable)
{
	if (Renderable)
	{
		ShowOnlyActors.Remove(Renderable->GetOwner());
	}
}
