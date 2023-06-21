// Copyright 2015-2020 Polyarc, Inc. All rights reserved.

#include "MikanCamera.h"
#include "Engine/Engine.h"
#include "MikanCaptureComponent.h"
#include "MikanWorldSubsystem.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "DrawDebugHelpers.h"

AMikanCamera::AMikanCamera(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	MikanCaptureComponent = CreateDefaultSubobject<UMikanCaptureComponent>(TEXT("SceneCaptureComponent2D"));
	MikanCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	RootComponent = MikanCaptureComponent;
}

void AMikanCamera::BeginPlay()
{
	Super::BeginPlay();

	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	if (MikanWorldSubsystem)
	{
		MikanWorldSubsystem->BindMikanCamera(this);
	}

	RebuildHiddenActorList();
}

void AMikanCamera::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DisposeRenderTarget();

	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	if (MikanWorldSubsystem)
	{
		MikanWorldSubsystem->UnbindCamera(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AMikanCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DrawDebugCamera(GetWorld(), GetActorLocation(), GetActorRotation(), MikanCaptureComponent->FOVAngle);
}

void AMikanCamera::SetCameraFOV(float FOVDegrees)
{
	MikanCaptureComponent->FOVAngle = FOVDegrees;
}

void AMikanCamera::RecreateRenderTarget(const MikanRenderTargetDescriptor& InRTDdesc)
{
	DisposeRenderTarget();

	check(InRTDdesc.color_buffer_type == MikanColorBuffer_BGRA32);
	RTDdesc= InRTDdesc;
	RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
		this, RTDdesc.width, RTDdesc.height, RTF_RGBA8, FLinearColor::Black, false);

	if (RenderTarget != nullptr)
	{
		RenderTarget->TargetGamma = GEngine->DisplayGamma;
		MikanCaptureComponent->TextureTarget = RenderTarget;
		MikanCaptureComponent->SetRenderTargetDesc(InRTDdesc);
	}
}

void AMikanCamera::DisposeRenderTarget()
{
	if (RenderTarget == nullptr)
		return;

	MikanCaptureComponent->TextureTarget = nullptr;
	UKismetRenderingLibrary::ReleaseRenderTarget2D(RenderTarget);
	RenderTarget = nullptr;
}

void AMikanCamera::RebuildHiddenActorList_Implementation()
{

}
