#include "MikanSceneActor.h"
#include "MikanAnchorActor.h"
#include "MikanAnchorTypes.h"
#include "MikanTransformActor.h"
#include "MikanSceneTypes.h"
#include "MikanBoxShapeActor.h"
#include "MikanBoxStencilActor.h"
#include "MikanDataStore.h"
#include "MikanEngineSubsystem.h"
#include "MikanMath.h"
#include "MikanModelShapeActor.h"
#include "MikanModelStencilActor.h"
#include "MikanQuadShapeActor.h"
#include "MikanQuadStencilActor.h"
#include "MikanShapeTypes.h"
#include "MikanStencilActor.h"
#include "MikanStencilTypes.h"
#include "MikanClient.h"
#include "MikanAPI.h"
#include "Engine/World.h"

// -- UMikanSceneData -----
void UMikanSceneData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanTransformData::Initialize(InValuesObject);

	const auto* SceneValues = InValuesObject.getTypedPointer<MikanSceneComponentValues>();
	DisplayCompositorId = SceneValues->display_compositor_id;
}

bool UMikanSceneData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "parent_stage_id")
	{
		ParentStageId = FieldValue.getIntValue();
		return true;
	}
	else if (FieldName == "display_compositor_id")
	{
		DisplayCompositorId = FieldValue.getIntValue();
		return true;
	}
	else
	{
		return UMikanTransformData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanSceneData::Describe(TArray<FString>& OutLines) const
{
	UMikanTransformData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("parent_stage_id"), ParentStageId);
	Mikan::AppendDescribeLine(OutLines, TEXT("display_compositor_id"), DisplayCompositorId);
}

// -- UMikanSceneActor -----
AMikanSceneActor::AMikanSceneActor(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
}

void AMikanSceneActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	AMikanClient* MikanClient = GetOwnerMikanClient();
	if (MikanClient)
	{
		if (!MikanClient->OnScriptMessage.IsAlreadyBound(this, &AMikanSceneActor::OnMikanScriptMessage))
		{
			MikanClient->OnScriptMessage.AddDynamic(this, &AMikanSceneActor::OnMikanScriptMessage);
		}
	}
}

void AMikanSceneActor::Destroyed()
{
	AMikanClient* MikanClient = GetOwnerMikanClient();
	if (MikanClient)
	{
		MikanClient->OnScriptMessage.RemoveDynamic(this, &AMikanSceneActor::OnMikanScriptMessage);
	}

	Super::Destroyed();
}

// Scene Events
void AMikanSceneActor::HandleSceneActivated()
{
	OnSceneActivated();
}

void AMikanSceneActor::HandleSceneDeactivated()
{
	OnSceneDeactivated();
}

// Script Message Events
void AMikanSceneActor::HandleScriptMessage(const FString& Message)
{
	OnMikanScriptMessage(Message);
}