#include "MikanModelStencilActor.h"
#include "MikanCamera.h"
#include "Engine/Engine.h"
#include "MikanAPI.h"
#include "MikanMath.h"
#include "MikanScene.h"
#include "MikanStencilRequests.h"
#include "MikanStencilTypes.h"
#include "MikanStencilComponent.h"
#include "ProceduralMeshComponent.h"

AMikanModelStencilActor::AMikanModelStencilActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("StencilMesh"));
	MeshComponent->SetupAttachment(RootComponent);
}

AMikanModelStencilActor* AMikanModelStencilActor::SpawnStencil(
	AMikanScene* OwnerScene,
	const MikanStencilModelInfo& modelStencilInfo)
{
	// Spawn a new stencil
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = *FString::Printf(TEXT("ModelStencil_%d"), modelStencilInfo.stencil_id);
	SpawnParameters.Owner = OwnerScene;

	// Spawn the stencil actor
	UWorld* World = OwnerScene->GetWorld();
	AMikanModelStencilActor* NewStencil = 
		World->SpawnActor<AMikanModelStencilActor>(
			AMikanModelStencilActor::StaticClass(), 
			FTransform::Identity, 
			SpawnParameters);

	if (OwnerScene->ModelStencilMaterial != nullptr)
	{
		NewStencil->Material = OwnerScene->ModelStencilMaterial;
	}

	NewStencil->AttachToActor(OwnerScene, FAttachmentTransformRules::SnapToTargetIncludingScale);
	NewStencil->ApplyModelStencilInfo(modelStencilInfo);
	NewStencil->RefetchModelRenderGeometry();

	return NewStencil;
}

void AMikanModelStencilActor::ApplyModelStencilInfo(const MikanStencilModelInfo& InStencilInfo)
{
	StencilComponent->ApplyStencilInfo(
		InStencilInfo.stencil_id,
		InStencilInfo.stencil_name.getValue(),
		InStencilInfo.relative_transform);
}

void AMikanModelStencilActor::RefetchModelRenderGeometry()
{
	// Fetch the meshes for the stencil
	auto* MikanAPI= GetParentScene()->GetMikanAPI();

	GetModelStencilRenderGeometry request;
	request.stencilId = GetStencilId();

	auto Response = MikanAPI->sendRequest(request).get();
	if (Response->resultCode == MikanAPIResult::Success)
	{
		auto MeshResponse = std::static_pointer_cast<MikanStencilModelRenderGeometryResponse>(Response);

		ApplyModelRenderGeometry(MeshResponse->render_geometry);
	}
}

void AMikanModelStencilActor::ApplyModelRenderGeometry(const MikanStencilModelRenderGeometry& InModelInfo)
{
	const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;

	MeshComponent->ClearAllMeshSections();

	for (int32 SectionIndex= 0; SectionIndex < (int)InModelInfo.meshes.size(); ++SectionIndex)
	{
		const MikanTriagulatedMesh& meshData = InModelInfo.meshes[SectionIndex];

		TArray<FVector> UE4Vertices;
		UE4Vertices.SetNumUninitialized(meshData.vertices.size());
		for (int32 i = 0; i < (int32)meshData.vertices.size(); ++i)
		{
			UE4Vertices[i] = FMikanMath::MikanVector3fToFVector(meshData.vertices[i]) * MetersToUU;
		}

		TArray<FVector> UE4Normals;
		UE4Normals.SetNumUninitialized(meshData.normals.size());
		for (int32 i = 0; i < (int32)meshData.normals.size(); ++i)
		{
			UE4Normals[i] = FMikanMath::MikanVector3fToFVector(meshData.normals[i]);
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

		MeshComponent->CreateMeshSection(
			SectionIndex,
			UE4Vertices,
			UE4Triangles,
			UE4Normals,
			UE4Texels,
			TArray<FColor>(),
			TArray<FProcMeshTangent>(),
			false); // No collision

		if (Material != nullptr)
		{
			MeshComponent->SetMaterial(SectionIndex, Material);
		}
	}
}