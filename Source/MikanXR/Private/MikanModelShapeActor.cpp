#include "MikanModelShapeActor.h"
#include "Engine/Engine.h"
#include "MikanAPI.h"
#include "MikanClient.h"
#include "MikanEngineSubsystem.h"
#include "MikanMath.h"
#include "MikanSceneActor.h"
#include "MikanShapeRequests.h"
#include "MikanShapeTypes.h"
#include "MikanStencilTypes.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/WorldSettings.h"
#include "DrawDebugHelpers.h"

// -- UMikanModelShapeData -----
void UMikanModelShapeData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanShapeData::Initialize(InValuesObject);

	const auto* ModelShapeValues = InValuesObject.getTypedPointer<MikanModelShapeComponentValues>();
	Mikan::ToUnrealString(ModelShapeValues->model_path, ModelPath);
}

bool UMikanModelShapeData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "model_path")
	{
		Mikan::ToUnrealString(FieldValue, ModelPath);
		return true;
	}
	else
	{
		return UMikanShapeData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanModelShapeData::Describe(TArray<FString>& OutLines) const
{
	UMikanShapeData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("model_path"), ModelPath);
}

// -- AMikanModelShapeActor -----
AMikanModelShapeActor::AMikanModelShapeActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ColorMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ColorShapeMesh"));
	ColorMeshComponent->SetupAttachment(RootComponent);

	DepthMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("DepthShapeMesh"));
	DepthMeshComponent->SetupAttachment(RootComponent);
	DepthMeshComponent->bVisibleInSceneCaptureOnly = true;
}

void AMikanModelShapeActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Draw bounding box using the mesh component's bounds
	if (ColorMeshComponent && ColorMeshComponent->GetNumSections() > 0)
	{
		const FBoxSphereBounds Bounds = ColorMeshComponent->Bounds;
		DrawDebugBox(GetWorld(), Bounds.Origin, Bounds.BoxExtent, FColor::Cyan);
	}
}

void AMikanModelShapeActor::RefetchModelRenderGeometry()
{
	UMikanEngineSubsystem* Subsystem = UMikanEngineSubsystem::Get();
	if (!Subsystem)
	{
		return;
	}

	// Supersede any in-flight fetch (e.g. a rapid model_path change) so a stale response can't
	// clobber the newer geometry.
	Subsystem->CancelRequest(PendingGeometryRequestHandle);
	PendingGeometryRequestHandle = 0;

	GetModelShapeRenderGeometry request;
	request.shapeId = GetTransformId();

	// Capture a weak pointer, never a raw this: the response can arrive after the actor is gone.
	TWeakObjectPtr<AMikanModelShapeActor> WeakThis(this);
	PendingGeometryRequestHandle = Subsystem->SendRequestAsync(
		request,
		[WeakThis](const MikanResponsePtr& Response)
		{
			AMikanModelShapeActor* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			Self->PendingGeometryRequestHandle = 0;
			if (Response->resultCode == MikanAPIResult::Success)
			{
				auto MeshResponse = std::static_pointer_cast<MikanShapeModelRenderGeometryResponse>(Response);
				Self->ApplyModelRenderGeometry(MeshResponse->render_geometry);
			}
		});
}

void AMikanModelShapeActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMikanEngineSubsystem* Subsystem = UMikanEngineSubsystem::Get())
	{
		Subsystem->CancelRequest(PendingGeometryRequestHandle);
	}
	PendingGeometryRequestHandle = 0;

	Super::EndPlay(EndPlayReason);
}

void AMikanModelShapeActor::ApplyModelRenderGeometry(const MikanStencilModelRenderGeometry& InModelInfo)
{
	const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;

	ColorMeshComponent->ClearAllMeshSections();
	DepthMeshComponent->ClearAllMeshSections();

	for (int32 SectionIndex = 0; SectionIndex < (int)InModelInfo.meshes.size(); ++SectionIndex)
	{
		const MikanTriagulatedMesh& meshData = InModelInfo.meshes[SectionIndex];

		TArray<FVector> UE4DepthVertices;
		TArray<FVector> UE4ColorVertices;
		UE4DepthVertices.SetNumUninitialized(meshData.vertices.size());
		UE4ColorVertices.SetNumUninitialized(meshData.vertices.size());
		for (int32 i = 0; i < (int32)meshData.vertices.size(); ++i)
		{
			UE4DepthVertices[i] = FMikanMath::MikanVector3fToFVector(meshData.vertices[i]) * MetersToUU;
			UE4ColorVertices[i] = UE4DepthVertices[i];
		}

		TArray<FVector> UE4Normals;
		UE4Normals.SetNumUninitialized(meshData.normals.size());
		for (int32 i = 0; i < (int32)meshData.normals.size(); ++i)
		{
			UE4Normals[i] = FMikanMath::MikanVector3fToFVector(meshData.normals[i]);
		}

		if (UE4ColorVertices.Num() == UE4Normals.Num())
		{
			for (int32 i = 0; i < UE4ColorVertices.Num(); ++i)
			{
				UE4ColorVertices[i] += UE4Normals[i] * ColorMeshInflationAmountMM;
			}
		}

		TArray<FVector2D> UE4Texels;
		UE4Texels.SetNumUninitialized(meshData.texels.size());
		for (int32 i = 0; i < (int32)meshData.texels.size(); ++i)
		{
			UE4Texels[i] = FMikanMath::MikanVector2fToFVector2D(meshData.texels[i]);
		}

		TArray<int32> UE4Triangles;
		UE4Triangles.SetNumUninitialized(meshData.indices.size());
		for (int32 i = 0; i < (int32)meshData.indices.size(); ++i)
		{
			UE4Triangles[i] = meshData.indices[i];
		}

		ColorMeshComponent->CreateMeshSection(
			SectionIndex,
			UE4ColorVertices,
			UE4Triangles,
			UE4Normals,
			UE4Texels,
			TArray<FColor>(),
			TArray<FProcMeshTangent>(),
			false);

		if (ColorMaterial != nullptr)
		{
			ColorMeshComponent->SetMaterial(SectionIndex, ColorMaterial);
		}

		DepthMeshComponent->CreateMeshSection(
			SectionIndex,
			UE4DepthVertices,
			UE4Triangles,
			UE4Normals,
			UE4Texels,
			TArray<FColor>(),
			TArray<FProcMeshTangent>(),
			false);
		DepthMeshComponent->bVisibleInSceneCaptureOnly = true;

		if (DepthMaterial != nullptr)
		{
			DepthMeshComponent->SetMaterial(SectionIndex, DepthMaterial);
		}
	}
}

void AMikanModelShapeActor::BindMikanComponentData(class UMikanComponentData* Data)
{
	Super::BindMikanComponentData(Data);

	AMikanClient* MikanClient = GetOwnerMikanClient();
	if (MikanClient)
	{
		if (ColorMaterial == nullptr && MikanClient->ModelShapeColorMaterial != nullptr)
		{
			ColorMaterial = MikanClient->ModelShapeColorMaterial;
		}

		if (DepthMaterial == nullptr && MikanClient->ModelShapeDepthMaterial != nullptr)
		{
			DepthMaterial = MikanClient->ModelShapeDepthMaterial;
		}
	}

	RefetchModelRenderGeometry();
}

void AMikanModelShapeActor::OnComponentDataChanged(const FString& FieldName)
{
	if (FieldName == "model_path")
	{
		RefetchModelRenderGeometry();
	}
	else
	{
		Super::OnComponentDataChanged(FieldName);
	}
}
