#include "MikanCamera.h"
#include "Engine/Engine.h"
#include "MikanCaptureComponent.h"
#include "MikanAPI.h"
#include "MikanMath.h"
#include "MikanScene.h"
#include "MikanRenderTargetRequests.h"
#include "MikanVideoSourceRequests.h"
#include "MikanVideoSourceTypes.h"
#include "MikanWorldSubsystem.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "DrawDebugHelpers.h"

DECLARE_LOG_CATEGORY_EXTERN(MikanXR, Log, All);

AMikanCamera::AMikanCamera(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneOrigin"));
	RootComponent = CameraRoot;

	ColorCaptureComponent = CreateDefaultSubobject<UMikanCaptureComponent>(TEXT("ColorCaptureComponent"));
	ColorCaptureComponent->SetupAttachment(CameraRoot);
	ColorCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;

	DepthCaptureComponent = CreateDefaultSubobject<UMikanCaptureComponent>(TEXT("DepthCaptureComponent"));
	DepthCaptureComponent->SetupAttachment(CameraRoot);
	DepthCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneDepth; // Scene Z as float in R channel
}

void AMikanCamera::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	if (MikanWorldSubsystem)
	{
		MikanAPI = MikanWorldSubsystem->GetMikanAPI();
		ClientInfo= MikanWorldSubsystem->GetClientInfo();

		MikanWorldSubsystem->OnMikanConnected.AddDynamic(this, &AMikanCamera::HandleMikanConnected);
		MikanWorldSubsystem->OnMikanDisconnected.AddDynamic(this, &AMikanCamera::HandleMikanDisconnected);
		MikanWorldSubsystem->OnVideoSourceOpened.AddDynamic(this, &AMikanCamera::HandleVideoSourceOpened);
		MikanWorldSubsystem->OnVideoSourceModeChanged.AddDynamic(this, &AMikanCamera::HandleVideoSourceModeChanged);
		MikanWorldSubsystem->OnVideoSourceIntrinsicsChanged.AddDynamic(this, &AMikanCamera::HandleVideoSourceIntrinsicsChanged);
		MikanWorldSubsystem->OnNewFrameEvent.AddDynamic(this, &AMikanCamera::HandleNewVideoSourceFrame);

		// If we are already connected, handle the connection side effects
		if (MikanAPI->getIsConnected())
		{
			HandleMikanConnected();
		}
	}
}

void AMikanCamera::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DisposeRenderTargets();

	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	if (MikanWorldSubsystem)
	{
		MikanAPI = MikanWorldSubsystem->GetMikanAPI();

		MikanWorldSubsystem->OnMikanConnected.RemoveDynamic(this, &AMikanCamera::HandleMikanConnected);
		MikanWorldSubsystem->OnMikanDisconnected.RemoveDynamic(this, &AMikanCamera::HandleMikanDisconnected);
		MikanWorldSubsystem->OnVideoSourceOpened.RemoveDynamic(this, &AMikanCamera::HandleVideoSourceOpened);
		MikanWorldSubsystem->OnVideoSourceModeChanged.RemoveDynamic(this, &AMikanCamera::HandleVideoSourceModeChanged);
		MikanWorldSubsystem->OnVideoSourceIntrinsicsChanged.RemoveDynamic(this, &AMikanCamera::HandleVideoSourceIntrinsicsChanged);
		MikanWorldSubsystem->OnNewFrameEvent.RemoveDynamic(this, &AMikanCamera::HandleNewVideoSourceFrame);
	}

	Super::EndPlay(EndPlayReason);
}

void AMikanCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	DrawDebugCamera(GetWorld(), GetActorLocation(), GetActorRotation(), MonoIntrinsics.hfov);
}

AMikanScene* AMikanCamera::GetParentScene() const
{
	AActor *CameraOwner= GetOwner();
	if (CameraOwner != nullptr)
	{
		return Cast<AMikanScene>(CameraOwner);
	}

	return nullptr;
}

// Mikan API Events
void AMikanCamera::HandleMikanConnected()
{
	// Video source opening implies intrinsics have changed
	HandleVideoSourceIntrinsicsChanged();
}

void AMikanCamera::HandleMikanDisconnected()
{
	FreeRenderBuffers();
}

void AMikanCamera::HandleVideoSourceOpened()
{
	// Video source opening implies intrinsics have changed
	HandleVideoSourceIntrinsicsChanged();
}

void AMikanCamera::HandleVideoSourceModeChanged()
{
	// Video mode changing implies intrinsics have changed
	HandleVideoSourceIntrinsicsChanged();
}

void AMikanCamera::HandleVideoSourceIntrinsicsChanged()
{
	GetVideoSourceIntrinsics request;
	auto Response= MikanAPI->sendRequest(request).get();
	if (Response->resultCode == MikanAPIResult::Success)
	{
		auto VideoIntrinsicsResponse= std::static_pointer_cast<MikanVideoSourceIntrinsicsResponse>(Response);
		MonoIntrinsics = VideoIntrinsicsResponse->intrinsics.getMonoIntrinsics();
	
		ColorCaptureComponent->SetVideoSourceIntrinsics(MonoIntrinsics);
		DepthCaptureComponent->SetVideoSourceIntrinsics(MonoIntrinsics);

		// Recreate the render target if the resolution has changed
		if (ColorRenderTarget == nullptr ||
			RenderTargetDesc.width != MonoIntrinsics.pixel_width ||
			RenderTargetDesc.height != MonoIntrinsics.pixel_height)
		{
			ReallocateRenderBuffers();
		}
	}
}

void AMikanCamera::HandleNewVideoSourceFrame(int64 NewFrameIndex, const FTransform& SceneTransform)
{
	// Update the scene camera transform
	SetActorRelativeTransform(SceneTransform);

	MikanClientGraphicsApi api = RenderTargetDesc.graphicsAPI;
	if (api == MikanClientGraphicsApi_Direct3D9 ||
		api == MikanClientGraphicsApi_Direct3D11 ||
		api == MikanClientGraphicsApi_Direct3D12 ||
		api == MikanClientGraphicsApi_OpenGL)
	{
		ColorCaptureComponent->CaptureFrame(NewFrameIndex);
		DepthCaptureComponent->CaptureFrame(NewFrameIndex);
		LastRenderedFrameIndex = NewFrameIndex;

		PublishRenderTargetTextures frameInfo;
		frameInfo.frameIndex = NewFrameIndex;

		MikanAPI->sendRequest(frameInfo);
	}
}

// Render Buffer Management
void AMikanCamera::RecreateRenderTargets(const MikanRenderTargetDescriptor& InRenderTargetDesc)
{
	DisposeRenderTargets();

	check(InRenderTargetDesc.color_buffer_type == MikanColorBuffer_BGRA32);
	RenderTargetDesc= InRenderTargetDesc;

	ColorRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
		this, RenderTargetDesc.width, RenderTargetDesc.height, RTF_RGBA8, FLinearColor::Transparent, false);
	if (ColorRenderTarget != nullptr)
	{
		ColorRenderTarget->TargetGamma = GEngine->DisplayGamma;
		ColorCaptureComponent->TextureTarget = ColorRenderTarget;
		ColorCaptureComponent->SetRenderTargetDesc(InRenderTargetDesc);
	}

	if (RenderTargetDesc.depth_buffer_type == MikanDepthBuffer_FLOAT_SCENE_DEPTH)
	{
		DepthRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
			this, RenderTargetDesc.width, RenderTargetDesc.height, RTF_R32f, FLinearColor::Transparent, false);
		if (DepthRenderTarget != nullptr)
		{
			DepthCaptureComponent->TextureTarget = DepthRenderTarget;
			DepthCaptureComponent->SetRenderTargetDesc(InRenderTargetDesc);
		}
	}
}

void AMikanCamera::DisposeRenderTargets()
{
	if (ColorRenderTarget != nullptr)
	{
		ColorCaptureComponent->TextureTarget = nullptr;
		UKismetRenderingLibrary::ReleaseRenderTarget2D(ColorRenderTarget);
		ColorRenderTarget = nullptr;
	}

	if (DepthRenderTarget != nullptr)
	{
		DepthCaptureComponent->TextureTarget = nullptr;
		UKismetRenderingLibrary::ReleaseRenderTarget2D(DepthRenderTarget);
		DepthRenderTarget = nullptr;
	}
}

void AMikanCamera::FreeRenderBuffers()
{
	DisposeRenderTargets();

	FreeRenderTargetTextures request;
	MikanAPI->sendRequest(request);
}

void AMikanCamera::ReallocateRenderBuffers()
{
	// Clean up any previously allocated render targets
	FreeRenderBuffers();

	// Fetch the video source information from Mikan
	GetVideoSourceMode request;
	auto Response= MikanAPI->sendRequest(request).get();
	if (Response->resultCode == MikanAPIResult::Success)
	{
		auto ModeResponse= std::static_pointer_cast<MikanVideoSourceModeResponse>(Response);

		RenderTargetDesc.width = (uint32_t)ModeResponse->resolution_x;
		RenderTargetDesc.height = (uint32_t)ModeResponse->resolution_y;
		RenderTargetDesc.color_buffer_type = MikanColorBuffer_BGRA32;
		RenderTargetDesc.depth_buffer_type = MikanDepthBuffer_FLOAT_SCENE_DEPTH;
		RenderTargetDesc.graphicsAPI = ClientInfo.graphicsAPI;

		// Allocate shared render textures using the render target descriptor
		AllocateRenderTargetTextures allocateRequest;
		allocateRequest.descriptor = RenderTargetDesc;

		auto AllocateResponse= MikanAPI->sendRequest(allocateRequest).get();
		if (AllocateResponse->resultCode == MikanAPIResult::Success)
		{
			// Tell the active scene camera to recreate a matching render target
			RecreateRenderTargets(RenderTargetDesc);
		}
	}
}