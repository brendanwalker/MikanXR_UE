#include "MikanModelShapeActor.h"
#include "Engine/Engine.h"
#include "MikanAPI.h"
#include "MikanClient.h"
#include "MikanEngineSubsystem.h"
#include "MikanGlbGeometry.h"
#include "MikanMath.h"
#include "MikanSceneActor.h"
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
	Mikan::ToUnrealString(ModelShapeValues->geometry_url, GeometryUrl);
}

bool UMikanModelShapeData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
{
	if (FieldName == "model_path")
	{
		Mikan::ToUnrealString(FieldValue, ModelPath);
		return true;
	}
	else if (FieldName == "geometry_url")
	{
		Mikan::ToUnrealString(FieldValue, GeometryUrl);
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
	Mikan::AppendDescribeLine(OutLines, TEXT("geometry_url"), GeometryUrl);
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
	// Supersede any in-flight fetch (e.g. a rapid model_path change) so a stale response can't
	// clobber the newer geometry.
	MikanGlbGeometry::CancelFetch(PendingGeometryRequest);

	auto* ShapeData = Cast<UMikanModelShapeData>(GetTransformData());
	const FString Url = ShapeData != nullptr ? MikanGlbGeometry::MakeGeometryUrl(ShapeData->GetGeometryUrl()) : FString();
	if (Url.IsEmpty())
	{
		return;
	}

	// Capture a weak pointer, never a raw this: the response can arrive after the actor is gone.
	TWeakObjectPtr<AMikanModelShapeActor> WeakThis(this);
	PendingGeometryRequest = MikanGlbGeometry::FetchGeometry(
		Url,
		[WeakThis](bool bSuccess, const FMikanGlbGeometry& Geometry)
		{
			AMikanModelShapeActor* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			Self->PendingGeometryRequest.Reset();
			if (bSuccess)
			{
				Self->ApplyModelRenderGeometry(Geometry);
			}
		});
}

void AMikanModelShapeActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	MikanGlbGeometry::CancelFetch(PendingGeometryRequest);

	Super::EndPlay(EndPlayReason);
}

void AMikanModelShapeActor::ApplyModelRenderGeometry(const FMikanGlbGeometry& Geometry)
{
	const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;

	ColorMeshComponent->ClearAllMeshSections();
	DepthMeshComponent->ClearAllMeshSections();

	for (int32 SectionIndex = 0; SectionIndex < Geometry.Meshes.Num(); ++SectionIndex)
	{
		const FMikanGlbMesh& meshData = Geometry.Meshes[SectionIndex];

		TArray<FVector> UE4DepthVertices;
		TArray<FVector> UE4ColorVertices;
		UE4DepthVertices.SetNumUninitialized(meshData.Positions.Num());
		UE4ColorVertices.SetNumUninitialized(meshData.Positions.Num());
		for (int32 i = 0; i < meshData.Positions.Num(); ++i)
		{
			const FVector3f& Position = meshData.Positions[i];
			UE4DepthVertices[i] = FMikanMath::MikanVector3fToFVector({Position.X, Position.Y, Position.Z}) * MetersToUU;
			UE4ColorVertices[i] = UE4DepthVertices[i];
		}

		TArray<FVector> UE4Normals;
		UE4Normals.SetNumUninitialized(meshData.Normals.Num());
		for (int32 i = 0; i < meshData.Normals.Num(); ++i)
		{
			const FVector3f& Normal = meshData.Normals[i];
			UE4Normals[i] = FMikanMath::MikanVector3fToFVector({Normal.X, Normal.Y, Normal.Z});
		}

		if (UE4ColorVertices.Num() == UE4Normals.Num())
		{
			for (int32 i = 0; i < UE4ColorVertices.Num(); ++i)
			{
				UE4ColorVertices[i] += UE4Normals[i] * ColorMeshInflationAmountMM;
			}
		}

		// glTF stores V-down, which is how Unreal samples, so UVs pass through
		TArray<FVector2D> UE4Texels;
		UE4Texels.SetNumUninitialized(meshData.UVs.Num());
		for (int32 i = 0; i < meshData.UVs.Num(); ++i)
		{
			UE4Texels[i] = FVector2D(meshData.UVs[i]);
		}

		TArray<int32> UE4Triangles;
		UE4Triangles.SetNumUninitialized(meshData.Indices.Num());
		for (int32 i = 0; i < meshData.Indices.Num(); ++i)
		{
			UE4Triangles[i] = (int32)meshData.Indices[i];
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
	// geometry_url changes after the editor has reloaded the model, so it is the field that means
	// the geometry moved
	if (FieldName == "geometry_url")
	{
		RefetchModelRenderGeometry();
	}
	else
	{
		Super::OnComponentDataChanged(FieldName);
	}
}
