#include "MikanAnchorActor.h"
#include "MikanCamera.h"
#include "Engine/Engine.h"
#include "MikanScene.h"
#include "MikanAnchorComponent.h"
#include "Components/TextRenderComponent.h"

AMikanAnchorActor::AMikanAnchorActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	AnchorComponent = CreateDefaultSubobject<UMikanAnchorComponent>(TEXT("Anchor"));
	RootComponent = AnchorComponent;
	
	LabelComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));;
	LabelComponent->SetupAttachment(RootComponent);
	LabelComponent->SetHorizontalAlignment(EHTA_Center);
}

AMikanScene* AMikanAnchorActor::GetParentScene() const
{
	USceneComponent* AttachParentComponent = RootComponent->GetAttachParent();

	if (AttachParentComponent != nullptr)
	{
		return Cast<AMikanScene>(AttachParentComponent->GetOwner());
	}

	return nullptr;
}

int32 AMikanAnchorActor::GetAnchorId() const
{
	return AnchorComponent->AnchorId;
}

const FString& AMikanAnchorActor::GetAnchorName() const
{
	return AnchorComponent->AnchorName;
}

void AMikanAnchorActor::UnbindAnchorId()
{
	AnchorComponent->AnchorId = -1;
}

void AMikanAnchorActor::ApplyAnchorInfo(const struct MikanSpatialAnchorInfo& InAnchorInfo)
{
	AnchorComponent->ApplyAnchorInfo(
		InAnchorInfo.anchor_id,
		InAnchorInfo.world_transform);
	UpdateLabelText();
}

void AMikanAnchorActor::UpdateLabelText()
{
	LabelComponent->SetText(FText::FromString(AnchorComponent->AnchorName));
}