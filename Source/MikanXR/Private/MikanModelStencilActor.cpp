#include "MikanModelStencilActor.h"
#include "Engine/Engine.h"
#include "MikanAPI.h"
#include "MikanCaptureComponent.h"
#include "MikanClient.h"
#include "MikanEngineSubsystem.h"
#include "MikanGlbGeometry.h"
#include "MikanMath.h"
#include "MikanRenderableComponent.h"
#include "MikanSceneActor.h"
#include "MikanStencilTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/WorldSettings.h"
#include "DrawDebugHelpers.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"

// -- UMikanModelStencilData -----
void UMikanModelStencilData::Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject)
{
	UMikanStencilData::Initialize(InValuesObject);

	const auto* ModelStencilValues = InValuesObject.getTypedPointer<MikanModelStencilComponentValues>();
	Mikan::ToUnrealString(ModelStencilValues->model_path, ModelPath);
	Mikan::ToUnrealString(ModelStencilValues->geometry_url, GeometryUrl);
}

bool UMikanModelStencilData::ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue)
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
		return UMikanStencilData::ApplyMikanValue(FieldName, FieldValue);
	}
}

void UMikanModelStencilData::Describe(TArray<FString>& OutLines) const
{
	UMikanStencilData::Describe(OutLines);
	Mikan::AppendDescribeLine(OutLines, TEXT("model_path"), ModelPath);
	Mikan::AppendDescribeLine(OutLines, TEXT("geometry_url"), GeometryUrl);
}

// -- AMikanModelStencilActor -----
AMikanModelStencilActor::AMikanModelStencilActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Visible placement reference (editor + PIE), never in a capture. Untagged.
	PlacementMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlacementStencilMesh"));
	PlacementMeshComponent->SetupAttachment(RootComponent);
	PlacementMeshComponent->bHiddenInSceneCapture = true;
	// The stencil mesh is built at runtime with no CPU-side copy, so it has no collision data to
	// cook. Leaving collision on makes the physics cook ask for triangles that are not there.
	PlacementMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlacementMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);

	// Capture-only; untagged by default (renders nowhere). bRenderColorMeshInColorTarget opts it into
	// the color capture via OnConstruction.
	ColorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ColorStencilMesh"));
	ColorMeshComponent->SetupAttachment(RootComponent);
	ColorMeshComponent->bVisibleInSceneCaptureOnly= true;
	ColorMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ColorMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);

	// Capture-only holdout occluder in the color pass: writes depth, outputs alpha 0.
	HoldoutMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HoldoutStencilMesh"));
	HoldoutMeshComponent->SetupAttachment(RootComponent);
	HoldoutMeshComponent->bVisibleInSceneCaptureOnly= true;
	HoldoutMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HoldoutMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HoldoutMeshComponent->ComponentTags.Add(UMikanCaptureComponent::MikanRenderColor);
	HoldoutMeshComponent->SetHoldout(true);
	HoldoutMeshComponent->SetCastShadow(false);

	DepthMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DepthStencilMesh"));
	DepthMeshComponent->SetupAttachment(RootComponent);
	DepthMeshComponent->bVisibleInSceneCaptureOnly= true;
	DepthMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DepthMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	DepthMeshComponent->ComponentTags.Add(UMikanCaptureComponent::MikanRenderDepth);
	DepthMeshComponent->SetCastShadow(false);

	ShadowMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShadowStencilMesh"));
	ShadowMeshComponent->SetupAttachment(RootComponent);
	ShadowMeshComponent->bVisibleInSceneCaptureOnly= true;
	ShadowMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShadowMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	ShadowMeshComponent->ComponentTags.Add(UMikanCaptureComponent::MikanRenderShadow);
	ShadowMeshComponent->SetCastShadow(false);

	MikanRenderableComponent= CreateDefaultSubobject<UMikanRenderableComponent>(TEXT("MikanRenderable"));
}

void AMikanModelStencilActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Opt the color mesh into the color capture only when requested. Done here (not the constructor)
	// so Blueprint / per-instance values of the flag are honored, and before BeginPlay registers the
	// renderable (when the capture reads these tags).
	if (ColorMeshComponent)
	{
		if (bRenderColorMeshInColorTarget)
		{
			ColorMeshComponent->ComponentTags.AddUnique(UMikanCaptureComponent::MikanRenderColor);
		}
		else
		{
			ColorMeshComponent->ComponentTags.Remove(UMikanCaptureComponent::MikanRenderColor);
		}
	}
}

void AMikanModelStencilActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Draw bounding box using the mesh component's bounds
	if (ColorMeshComponent && ColorMeshComponent->GetStaticMesh() != nullptr)
	{
		const FBoxSphereBounds Bounds = ColorMeshComponent->Bounds;
		DrawDebugBox(GetWorld(), Bounds.Origin, Bounds.BoxExtent, FColor::Yellow);
	}
}

void AMikanModelStencilActor::RefetchModelRenderGeometry()
{
	// Supersede any in-flight fetch (e.g. a rapid model_path change) so a stale response can't
	// clobber the newer geometry.
	MikanGlbGeometry::CancelFetch(PendingGeometryRequest);

	auto* StencilData = Cast<UMikanModelStencilData>(GetTransformData());
	const FString Url = StencilData != nullptr ? MikanGlbGeometry::MakeGeometryUrl(StencilData->GetGeometryUrl()) : FString();
	if (Url.IsEmpty())
	{
		return;
	}

	// Capture a weak pointer, never a raw this: the response can arrive after the actor is gone.
	TWeakObjectPtr<AMikanModelStencilActor> WeakThis(this);
	PendingGeometryRequest = MikanGlbGeometry::FetchGeometry(
		Url,
		[WeakThis](bool bSuccess, const FMikanGlbGeometry& Geometry)
		{
			AMikanModelStencilActor* Self = WeakThis.Get();
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

void AMikanModelStencilActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	MikanGlbGeometry::CancelFetch(PendingGeometryRequest);

	Super::EndPlay(EndPlayReason);
}

namespace
{
// The fast static mesh build copies the tangent frame straight out of the mesh description without
// recomputing it, and Mikan does not ship tangents. The stencil materials are unlit color, depth
// only, a holdout occluder and a plain lambertian shadow catcher, and the textured placement
// material samples a single base texture, so none of them read a real tangent - but leaving the
// tangent zero hands the renderer a degenerate basis, so an arbitrary perpendicular is used.
FVector3f MakeTangentForNormal(const FVector3f& Normal)
{
	const FVector3f Reference =
		FMath::Abs(Normal.Z) < 0.99f ? FVector3f(0.f, 0.f, 1.f) : FVector3f(1.f, 0.f, 0.f);

	return FVector3f::CrossProduct(Reference, Normal).GetSafeNormal();
}
} // namespace

UStaticMesh* AMikanModelStencilActor::BuildStaticMeshFromRenderGeometry(const FMikanGlbGeometry& Geometry)
{
	const float MetersToUU = GetWorld()->GetWorldSettings()->WorldToMeters;

	FMeshDescription MeshDescription;
	FStaticMeshAttributes Attributes(MeshDescription);
	Attributes.Register();

	TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> InstanceNormals = Attributes.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector3f> InstanceTangents = Attributes.GetVertexInstanceTangents();
	TVertexInstanceAttributesRef<float> InstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
	TVertexInstanceAttributesRef<FVector2f> InstanceUVs = Attributes.GetVertexInstanceUVs();
	TPolygonGroupAttributesRef<FName> MaterialSlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
	InstanceUVs.SetNumChannels(1);

	TArray<FStaticMaterial> StaticMaterials;

	for (int32 SectionIndex = 0; SectionIndex < Geometry.Meshes.Num(); ++SectionIndex)
	{
		const FMikanGlbMesh& MeshData = Geometry.Meshes[SectionIndex];
		const int32 VertexCount = MeshData.Positions.Num();
		const int32 IndexCount = MeshData.Indices.Num();
		if (VertexCount == 0 || IndexCount < 3)
		{
			continue;
		}

		// One polygon group per incoming mesh, so a section keeps its own material slot. The build
		// resolves a section's material by matching this slot name against the static material list.
		const FPolygonGroupID PolygonGroupID = MeshDescription.CreatePolygonGroup();
		const FName MaterialSlotName = *FString::Printf(TEXT("Section%d"), SectionIndex);
		MaterialSlotNames[PolygonGroupID] = MaterialSlotName;
#if WITH_EDITORONLY_DATA
		StaticMaterials.Add(
			FStaticMaterial(UMaterial::GetDefaultMaterial(MD_Surface), MaterialSlotName, MaterialSlotName));
#else
		StaticMaterials.Add(FStaticMaterial(UMaterial::GetDefaultMaterial(MD_Surface), MaterialSlotName));
#endif

		TArray<FVertexID> VertexIDs;
		VertexIDs.Reserve(VertexCount);
		for (int32 i = 0; i < VertexCount; ++i)
		{
			const FVertexID VertexID = MeshDescription.CreateVertex();
			VertexPositions[VertexID] = (FVector3f)(FMikanMath::MikanVector3fToFVector(
															 {MeshData.Positions[i].X, MeshData.Positions[i].Y,
															  MeshData.Positions[i].Z}) *
														 MetersToUU);
			VertexIDs.Add(VertexID);
		}

		const bool bHasNormals = MeshData.Normals.Num() == VertexCount;
		const bool bHasUVs = MeshData.UVs.Num() == VertexCount;

		for (int32 i = 0; i + 2 < IndexCount; i += 3)
		{
			// Reject the whole triangle before creating anything, so a bad index can't leave
			// orphaned vertex instances behind in the description.
			const uint32 TriangleIndices[3] = {MeshData.Indices[i], MeshData.Indices[i + 1], MeshData.Indices[i + 2]};
			if (TriangleIndices[0] >= (uint32)VertexCount || TriangleIndices[1] >= (uint32)VertexCount ||
				TriangleIndices[2] >= (uint32)VertexCount)
			{
				continue;
			}

			FVertexInstanceID Corners[3];
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const int32 VertexIndex = (int32)TriangleIndices[Corner];
				const FVertexInstanceID InstanceID = MeshDescription.CreateVertexInstance(VertexIDs[VertexIndex]);

				const FVector3f& MikanNormal = bHasNormals ? MeshData.Normals[VertexIndex] : FVector3f::ZeroVector;
				const FVector3f Normal =
					bHasNormals
						? (FVector3f)FMikanMath::MikanVector3fToFVector({MikanNormal.X, MikanNormal.Y, MikanNormal.Z})
						: FVector3f(0.f, 0.f, 1.f);
				InstanceNormals[InstanceID] = Normal;
				InstanceTangents[InstanceID] = MakeTangentForNormal(Normal);
				InstanceBinormalSigns[InstanceID] = 1.f;

				// glTF stores V-down, which is how Unreal samples, so UVs pass through
				const FVector2f UV = bHasUVs ? MeshData.UVs[VertexIndex] : FVector2f::ZeroVector;
				InstanceUVs.Set(InstanceID, 0, UV);

				Corners[Corner] = InstanceID;
			}

			// Index triples are passed through in arrival order, the same as the procedural mesh
			// path did, and a mesh description winds a triangle the same way, so facing is unchanged.
			MeshDescription.CreateTriangle(PolygonGroupID, Corners);
		}
	}

	if (MeshDescription.Triangles().Num() == 0)
	{
		return nullptr;
	}

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(this, NAME_None, RF_Transient);
	StaticMesh->SetStaticMaterials(StaticMaterials);
	StaticMesh->NeverStream = true;

	// The stencil is drawn, never collided with, and the build keeps no CPU copy of the triangles.
	// Without this the body setup still advertises trimesh collision and the physics cook warns
	// every time it asks for data that was never kept.
	StaticMesh->CreateBodySetup();
	if (UBodySetup* BodySetup = StaticMesh->GetBodySetup())
	{
		BodySetup->bNeverNeedsCookedCollisionData = true;
		BodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
	}

	UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
	BuildParams.bMarkPackageDirty = false;
	BuildParams.bBuildSimpleCollision = false;
	// The mesh is rebuilt from the server on every fetch, so there is nothing to gain from keeping
	// an editable description around, and bFastBuild is what makes this usable at runtime.
	BuildParams.bCommitMeshDescription = false;
	BuildParams.bFastBuild = true;

	if (!StaticMesh->BuildFromMeshDescriptions({&MeshDescription}, BuildParams))
	{
		UE_LOG(LogTemp, Warning, TEXT("MikanModelStencilActor: failed to build stencil static mesh"));
		return nullptr;
	}

	return StaticMesh;
}

void AMikanModelStencilActor::ApplyStencilMeshToComponent(UStaticMeshComponent* MeshComponent,
														  UMaterialInterface* Material, int32 SectionCount)
{
	if (MeshComponent == nullptr)
	{
		return;
	}

	MeshComponent->SetStaticMesh(StencilMesh);

	if (Material != nullptr)
	{
		for (int32 SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
		{
			MeshComponent->SetMaterial(SectionIndex, Material);
		}
	}
}

void AMikanModelStencilActor::ApplyModelRenderGeometry(const FMikanGlbGeometry& Geometry)
{
	// The placement preview shows the first textured mesh's image. Depth proxy and occlusion mesh
	// captures are one mesh with one texture.
	PlacementTexture = nullptr;
	for (const FMikanGlbMesh& Mesh : Geometry.Meshes)
	{
		if (Geometry.Images.IsValidIndex(Mesh.ImageIndex))
		{
			PlacementTexture = MikanGlbGeometry::CreateTexture(Geometry.Images[Mesh.ImageIndex]);
			break;
		}
	}

	StencilMesh = BuildStaticMeshFromRenderGeometry(Geometry);

	const int32 SectionCount = StencilMesh != nullptr ? StencilMesh->GetStaticMaterials().Num() : 0;

	// Prefer the stencil's own texture when its geometry carries one and a textured parent material
	// is assigned; otherwise fall back to the plain color material.
	UMaterialInterface* PlacementMaterial = ColorMaterial;
	if (PlacementTexture != nullptr && TexturedPlacementMaterial != nullptr)
	{
		UMaterialInstanceDynamic* TexturedMID = UMaterialInstanceDynamic::Create(TexturedPlacementMaterial, this);
		TexturedMID->SetTextureParameterValue(TEXT("BaseTexture"), PlacementTexture);
		PlacementMaterial = TexturedMID;
	}

	// Every pass renders the same mesh resource and differs only in material and the visibility
	// flags set in the constructor. The holdout occluder keeps the mesh's default material, which
	// writes depth while the holdout flag suppresses its color.
	ApplyStencilMeshToComponent(PlacementMeshComponent, PlacementMaterial, SectionCount);
	ApplyStencilMeshToComponent(ColorMeshComponent, ColorMaterial, SectionCount);
	ApplyStencilMeshToComponent(HoldoutMeshComponent, nullptr, SectionCount);
	ApplyStencilMeshToComponent(DepthMeshComponent, DepthMaterial, SectionCount);
	ApplyStencilMeshToComponent(ShadowMeshComponent, ShadowMaterial, SectionCount);
}

void AMikanModelStencilActor::BindMikanComponentData(class UMikanComponentData* Data)
{
	Super::BindMikanComponentData(Data);

	AMikanClient* MikanClient= GetOwnerMikanClient();
	if (MikanClient)
	{
		if (ColorMaterial == nullptr && MikanClient->ModelStencilColorMaterial != nullptr)
		{
			ColorMaterial = MikanClient->ModelStencilColorMaterial;
		}

		if (DepthMaterial == nullptr && MikanClient->ModelStencilDepthMaterial != nullptr)
		{
			DepthMaterial = MikanClient->ModelStencilDepthMaterial;
		}

		if (ShadowMaterial == nullptr && MikanClient->ModelStencilShadowMaterial != nullptr)
		{
			ShadowMaterial = MikanClient->ModelStencilShadowMaterial;
		}

		if (TexturedPlacementMaterial == nullptr && MikanClient->ModelStencilTexturedPlacementMaterial != nullptr)
		{
			TexturedPlacementMaterial = MikanClient->ModelStencilTexturedPlacementMaterial;
		}
	}

	RefetchModelRenderGeometry();
}

void AMikanModelStencilActor::OnComponentDataChanged(const FString& FieldName)
{
	// geometry_url changes after the editor has reloaded the model, and its etag query changes with
	// the model or texture files, so it is the one field that means the geometry moved
	if (FieldName == "geometry_url")
	{
		RefetchModelRenderGeometry();
	}
	else
	{
		Super::OnComponentDataChanged(FieldName);
	}
}
