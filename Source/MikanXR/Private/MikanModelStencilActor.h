#pragma once

#include "GameFramework/Actor.h"
#include "MikanTransformActor.h"
#include "MikanStencilActor.h"
#include "MikanStencilTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "MikanModelStencilActor.generated.h"

UCLASS()
class UMikanModelStencilData : public UMikanStencilData
{
	GENERATED_BODY()

public:
	UMikanModelStencilData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline const FString& GetModelPath() const { return ModelPath; }
	inline const FString& GetGeometryUrl() const { return GeometryUrl; }

private:
	FString ModelPath;
	FString GeometryUrl;
};

UCLASS(BlueprintType, Blueprintable)
class AMikanModelStencilActor : public AMikanStencilActor
{
	GENERATED_BODY()

public:
	AMikanModelStencilActor(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	// Every mesh component below shares one StencilMesh. They differ only in material and
	// visibility flags, so the fetched geometry is uploaded once rather than once per pass.

	// Visible in the editor and PIE for placement / navigating relative to the stencil. Not in any
	// Mikan capture (untagged), so it never appears in a render target.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* PlacementMeshComponent;

	// Capture-only by default and untagged, so it renders nowhere. Opt in via bRenderColorMeshInColorTarget
	// to tag it MikanRenderColor and draw the stencil's actual color into the color render target.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* ColorMeshComponent;

	// Capture-only holdout occluder for the color pass: writes scene depth (so the character behind
	// the stencil is cut out) but outputs alpha 0 (invisible), letting the real surface show through.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* HoldoutMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* DepthMeshComponent;

	// Shadow catcher mesh, rendered only by the shadow capture. Receives cast shadows with a
	// white catcher material so the shadow pass produces the per-channel shadow (A/B) factor.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* ShadowMeshComponent;

	// MikanCaptureComponent looks for actors with MikanRenderableComponent to gather meshes
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UMikanRenderableComponent* MikanRenderableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* ColorMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* DepthMaterial;

	// Pure Lambertian Diffuse material
	// * Base Color = white(1, 1, 1)
	// * Metallic = 0, Roughness = 1, Specular = 0
	// * Default Lit, Opaque, Receives shadows(default), Casts no shadow itself
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* ShadowMaterial;

	// Parent material for texturing the placement mesh with the stencil's own texture (the captured
	// frame of a depth proxy or the atlas of an occlusion mesh), which arrives embedded in the
	// stencil's geometry GLB. Needs a Texture parameter named "BaseTexture". When unset, or when the
	// geometry carries no texture, the placement mesh uses ColorMaterial. Not defaulted to a /Game/ asset - the plugin is shared
	// across projects (same reasoning as the light environment's skydome material).
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* TexturedPlacementMaterial;

	// When true, ColorMeshComponent is tagged MikanRenderColor so the stencil's color is drawn into
	// the color render target (in addition to the holdout occluder). Off by default. Applied in
	// OnConstruction so Blueprint / instance overrides are honored before renderable registration.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bRenderColorMeshInColorTarget = false;

	virtual void BindMikanComponentData(class UMikanComponentData* Data) override;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void ApplyModelRenderGeometry(const struct FMikanGlbGeometry& Geometry);
	void RefetchModelRenderGeometry();

	// Builds one transient UStaticMesh from the fetched geometry, one section per incoming mesh.
	// Returns null when the geometry has no usable triangles.
	class UStaticMesh* BuildStaticMeshFromRenderGeometry(const struct FMikanGlbGeometry& Geometry);

	// Points one pass's component at StencilMesh and overrides every section with Material. A null
	// Material leaves the mesh's own materials in place, which is what the holdout occluder wants.
	void ApplyStencilMeshToComponent(class UStaticMeshComponent* MeshComponent, class UMaterialInterface* Material,
									 int32 SectionCount);

	virtual void OnComponentDataChanged(const FString& FieldName) override;

	// The in-flight geometry GLB fetch, if any. Superseded when geometry_url changes again and
	// cancelled on EndPlay.
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> PendingGeometryRequest;

	// The first textured mesh's base colour texture from the geometry GLB, if any.
	UPROPERTY(Transient)
	class UTexture2D* PlacementTexture = nullptr;

	// The one mesh resource every pass component renders. Rebuilt on each fetch and held here so it
	// stays rooted for as long as the components reference it.
	UPROPERTY(Transient)
	class UStaticMesh* StencilMesh = nullptr;
};
