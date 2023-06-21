#include "MikanAnchorComponent.h"
#include "MikanWorldSubsystem.h"
#include "MikanClient_CAPI.h"
#include "MikanMath.h"

UMikanAnchorComponent::UMikanAnchorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	this->SetRelativeScale3D(FVector(1.f));
	this->SetRelativeLocation(FVector::ZeroVector);
}

void UMikanAnchorComponent::BeginPlay()
{
	Super::BeginPlay();

	auto* MikanWorldSubsystem= UMikanWorldSubsystem::GetInstance(GetWorld());
	MikanWorldSubsystem->RegisterAnchor(this);
	MikanWorldSubsystem->OnMikanConnected.AddDynamic(this, &UMikanAnchorComponent::NotifyMikanConnected);

	FindAnchorInfo();
}

void UMikanAnchorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	auto* MikanWorldSubsystem = UMikanWorldSubsystem::GetInstance(GetWorld());
	MikanWorldSubsystem->UnregisterAnchor(this);
	MikanWorldSubsystem->OnMikanConnected.RemoveDynamic(this, &UMikanAnchorComponent::NotifyMikanConnected);

	Super::EndPlay(EndPlayReason);
}

void UMikanAnchorComponent::NotifyMikanConnected()
{
	FindAnchorInfo();
}

void UMikanAnchorComponent::FindAnchorInfo()
{
	if (Mikan_GetIsConnected())
	{
		MikanSpatialAnchorInfo AnchorInfo;
		if (Mikan_FindSpatialAnchorInfoByName(TCHAR_TO_ANSI(*AnchorName), &AnchorInfo) == MikanResult_Success)
		{
			const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;
			const FTransform RelativeTransform =
				FMikanMath::MikanTransformToFTransform(
					AnchorInfo.relative_transform, MetersToUU);

			AnchorId= AnchorInfo.anchor_id;
			SetRelativeTransform(RelativeTransform);
		}
	}
}
