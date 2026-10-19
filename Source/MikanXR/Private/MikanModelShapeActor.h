#pragma once

#include "MikanTransformActor.h"
#include "MikanShapeActor.h"
#include "MikanShapeTypes.h"
#include "MikanStencilTypes.h"
#include "ProceduralMeshComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "MikanModelShapeActor.generated.h"

UCLASS()
class UMikanModelShapeData : public UMikanShapeData
{
	GENERATED_BODY()

public:
	UMikanModelShapeData() = default;

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
class AMikanModelShapeActor : public AMikanShapeActor
{
	GENERATED_BODY()

public:
	AMikanModelShapeActor(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UProceduralMeshComponent* ColorMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UProceduralMeshComponent* DepthMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* ColorMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* DepthMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ColorMeshInflationAmountMM = 0.1f;

	virtual void BindMikanComponentData(class UMikanComponentData* Data) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void ApplyModelRenderGeometry(const struct FMikanGlbGeometry& Geometry);
	void RefetchModelRenderGeometry();

	virtual void OnComponentDataChanged(const FString& FieldName) override;

	// The in-flight geometry GLB fetch, if any. Superseded when geometry_url changes again and
	// cancelled on EndPlay.
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> PendingGeometryRequest;
};
