#include "MikanCaptureComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/Material.h"
#include "MikanWorldSubsystem.h"
#include "MikanRenderTargetRequests.h"
#include "MikanRenderableComponent.h"
#include "MikanAPI.h"

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

	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	if (MikanWorldSubsystem)
	{
		MikanAPI = MikanWorldSubsystem->GetMikanAPI();

		MikanWorldSubsystem->OnRenderableRegistered.AddDynamic(this, &UMikanCaptureComponent::HandleMikanRenderableRegistered);
		MikanWorldSubsystem->OnRenderableUnregistered.AddDynamic(this, &UMikanCaptureComponent::HandleMikanRenderableUnregistered);

		// Register all existing renderables
		const auto& ExistingRenderables= MikanWorldSubsystem->GetRegisteredMikanRenderables();
		for (UMikanRenderableComponent* Renderable : ExistingRenderables)
		{
			HandleMikanRenderableRegistered(Renderable);
		}	
	}
}

void UMikanCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	if (MikanWorldSubsystem)
	{
		MikanWorldSubsystem->OnRenderableRegistered.RemoveDynamic(this, &UMikanCaptureComponent::HandleMikanRenderableRegistered);
		MikanWorldSubsystem->OnRenderableUnregistered.RemoveDynamic(this, &UMikanCaptureComponent::HandleMikanRenderableUnregistered);
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

void UMikanCaptureComponent::SetVideoSourceIntrinsics(const MikanMonoIntrinsics& InMonoIntrinsics)
{
	UWorld* World = GetWorld();
	const float MetersToUU = World->GetWorldSettings()->WorldToMeters;
	const float HalfXFOV = FMath::DegreesToRadians(InMonoIntrinsics.hfov) * 0.5f;
	const float HalfYFOV = FMath::DegreesToRadians(InMonoIntrinsics.vfov) * 0.5f;
	
	NearClippingPlaneUU= InMonoIntrinsics.znear * MetersToUU;
	FarClippingPlaneUU= InMonoIntrinsics.zfar * MetersToUU;

	CustomProjectionMatrix= 
		FReversedZPerspectiveMatrix(
			HalfXFOV, HalfYFOV, 
			1.0f, 1.0, // FOV scales
			NearClippingPlaneUU, FarClippingPlaneUU);
	bUseCustomProjectionMatrix = true;

	VideoSourceIntrinsics = InMonoIntrinsics;
}

void UMikanCaptureComponent::CaptureFrame(uint64 NewFrameIndex)
{
	if (TextureTarget != nullptr)
	{
		CaptureScene();

		MikanClientGraphicsApi api = RenderTargetDesc.graphicsAPI;
		if (api == MikanClientGraphicsApi_Direct3D9 ||
			api == MikanClientGraphicsApi_Direct3D11 ||
			api == MikanClientGraphicsApi_Direct3D12 ||
			api == MikanClientGraphicsApi_OpenGL)
		{
			// TODO: I think this isn't thread safe and should be done in the render thread
			// But I'm not sure how to enqueue a render command that happens AFTER the scene capture
			// Currently if I try to enqueue a render command, I get frequent flicking
			UTextureRenderTarget2D* RenderTarget2D = TextureTarget;
			FTextureRenderTargetResource* TextureResource = RenderTarget2D->GameThread_GetRenderTargetResource();
			if (TextureResource != nullptr)
			{
				FRHITexture2D* TextureRHI = TextureResource->GetTexture2DRHI();
				if (TextureRHI != nullptr)
				{
					void* NativeTexturePtr = TextureRHI->GetNativeResource();
					if (NativeTexturePtr != nullptr)
					{					
						if (CaptureSource == ESceneCaptureSource::SCS_SceneColorHDR ||
							CaptureSource == ESceneCaptureSource::SCS_FinalColorLDR)
						{
							WriteColorRenderTargetTexture writeRequest;
							writeRequest.apiColorTexturePtr= NativeTexturePtr;

							MikanAPI->sendRequest(writeRequest);
						}
						else if (CaptureSource == ESceneCaptureSource::SCS_SceneDepth)
						{
							WriteDepthRenderTargetTexture writeRequest;
							writeRequest.apiDepthTexturePtr= NativeTexturePtr;
							writeRequest.zNear= NearClippingPlaneUU;
							writeRequest.zFar= FarClippingPlaneUU;

							MikanAPI->sendRequest(writeRequest);
						}
					}
				}
			}
		}
	}
}

// Mikan UE4 Events
void UMikanCaptureComponent::HandleMikanRenderableRegistered(UMikanRenderableComponent* Renderable)
{
	if (Renderable)
	{
		ShowOnlyActors.AddUnique(Renderable->GetOwner());
	}
}

void UMikanCaptureComponent::HandleMikanRenderableUnregistered(UMikanRenderableComponent* Renderable)
{
	if (Renderable)
	{
		ShowOnlyActors.Remove(Renderable->GetOwner());
	}
}