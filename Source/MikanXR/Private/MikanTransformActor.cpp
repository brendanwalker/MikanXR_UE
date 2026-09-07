#include "MikanTransformActor.h"
#include "MikanTransformTypes.h"
#include "MikanClient.h"
#include "MikanMath.h"
#include "MikanSceneActor.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Components/TextRenderComponent.h"

// -- UMikanTransformData -----
void UMikanTransformData::Initialize(
	const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanComponentData::Initialize(InValuesObject);

	const auto* ComponentValues = InValuesObject.getTypedPointer<MikanTransformComponentValues>();
	ParentTransformId = ComponentValues->parent_transform_id;
	RelativePosition = FMikanMath::MikanVector3fToFVector(ComponentValues->relative_position);
	RelativeRotation = FMikanMath::MikanQuatToFQuat(ComponentValues->relative_quaternion);
	RelativeScale = FMikanMath::MikanVector3fToFVector(ComponentValues->relative_scale);
}

bool UMikanTransformData::ApplyMikanValue(
	const FString& FieldName,
	const MikanVariant& FieldValue)
{
	if (FieldName == "parent_transform_id")
	{
		ParentTransformId = FieldValue.getIntValue();
		return true;
	}
	else if (FieldName == "relative_position")
	{
		RelativePosition = FMikanMath::MikanVector3fToFVector(FieldValue.getVector3fValue());
		return true;
	}
	else if (FieldName == "relative_quaternion")
	{
		RelativeRotation = FMikanMath::MikanQuatToFQuat(FieldValue.getQuaternionfValue());
		return true;
	}
	else if (FieldName == "relative_scale")
	{
		RelativeScale = FMikanMath::MikanVector3fToFVector(FieldValue.getVector3fValue());
		return true;
	}
	else
	{
		// If the field is not specific to the transform component, attempt to apply it as a generic component value
		return UMikanComponentData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanTransformData::Describe(TArray<FString>& OutLines) const
{
	UMikanComponentData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("parent_transform_id"), (int32)ParentTransformId);
	Mikan::AppendDescribeLine(OutLines, TEXT("relative_position"), RelativePosition);
	Mikan::AppendDescribeLine(OutLines, TEXT("relative_quaternion"), RelativeRotation);
	Mikan::AppendDescribeLine(OutLines, TEXT("relative_scale"), RelativeScale);
}

FTransform UMikanTransformData::ComputeRelativeTransform(UWorld* World) const
{
	const float MetersToUU = World->GetWorldSettings()->WorldToMeters;
	const FVector Location = GetRelativePosition() * MetersToUU;

	return FTransform(
		GetRelativeRotation(),
		Location,
		GetRelativeScale());
}

// -- UMikanTransformData -----
AMikanTransformActor::AMikanTransformActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	MikanTransform = CreateDefaultSubobject<USceneComponent>(TEXT("MikanTransform"));
	RootComponent = MikanTransform;

	LabelComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	LabelComponent->SetupAttachment(RootComponent);
	LabelComponent->SetHorizontalAlignment(EHTA_Center);
	LabelComponent->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
}

void AMikanTransformActor::Destroyed()
{
	Super::Destroyed();

	UnbindMikanComponentData(TransformData);
}

AMikanClient* AMikanTransformActor::GetOwnerMikanClient() const
{
	return Cast<AMikanClient>(GetOwner());
}

void AMikanTransformActor::BindMikanComponentData(UMikanComponentData* Data)
{
	auto* NewTransformData = Cast<UMikanTransformData>(Data);
	if (NewTransformData && NewTransformData != TransformData)
	{
		// If we are already bound to transform data, unbind from it before binding to the new data
		UnbindMikanComponentData(TransformData);

		// Assign the new transform data to this actor
		TransformData = NewTransformData;

		// Listen for changes to the transform data so that we can update the actor's transform when the data changes
		TransformData->OnComponentDataChanged.AddUObject(this, &AMikanTransformActor::OnComponentDataChanged);

		// Apply all component data change side effects
		OnMikanComponentNameChanged();
		OnMikanTransformDataChanged();
		OnMikanAttachmentChanged();
	}
}

void AMikanTransformActor::UnbindMikanComponentData(class UMikanComponentData* Data)
{
	if (Data && Data == TransformData)
	{
		TransformData->OnComponentDataChanged.RemoveAll(this);
		TransformData = nullptr;
	}
}

void AMikanTransformActor::OnComponentDataChanged(const FString& FieldName)
{
	if (FieldName == "component_name")
	{
		OnMikanComponentNameChanged();
	}
	if (FieldName == "parent_transform_id")
	{
		OnMikanAttachmentChanged();
	}
	else if (FieldName == "relative_position" || 
			FieldName == "relative_quaternion" || 
			FieldName == "relative_scale")
	{
		OnMikanTransformDataChanged();
	}
}

void AMikanTransformActor::OnMikanComponentNameChanged()
{
	const FString& ComponentName= TransformData->GetComponentName();
	
	// Update the in world label
	LabelComponent->SetText(FText::FromString(ComponentName));
	
	// Also update the editor hierarchy view label
	SetActorLabel(ComponentName);
}

void AMikanTransformActor::OnMikanTransformDataChanged()
{
	UWorld* World = GetWorld();
	FTransform RelativeTransform= TransformData->ComputeRelativeTransform(World);

	SetActorRelativeTransform(RelativeTransform);
}

#if WITH_EDITOR
bool AMikanTransformActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AMikanTransformActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	// Snap back to Mikan-driven transform — non-stage actors are not user-movable.
	if (GetTransformData())
	{
		OnMikanTransformDataChanged();
	}
}
#endif

void AMikanTransformActor::OnMikanAttachmentChanged()
{
	AMikanClient* OwnerMikanClient= GetOwnerMikanClient();

	// Bail out if this actor is mid-teardown: a destroyed actor has its Owner cleared,
	// and an unbound actor has no TransformData. Either can happen transiently while the
	// client re-syncs spawned actors on reconnect.
	if (!OwnerMikanClient || !TransformData)
	{
		return;
	}

	AMikanTransformActor* NewParentTransformActor =
		OwnerMikanClient->GetTransformActorById(
			TransformData->GetParentTransformId());
	
	if (NewParentTransformActor != nullptr)
	{
		AttachToActor(NewParentTransformActor, FAttachmentTransformRules::KeepRelativeTransform);
		ParentTransformActor = NewParentTransformActor;
	}
	else
	{
		AttachToActor(OwnerMikanClient, FAttachmentTransformRules::KeepRelativeTransform);
		ParentTransformActor = nullptr;
	}
}
