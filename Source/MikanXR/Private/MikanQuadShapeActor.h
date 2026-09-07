#pragma once

#include "MikanTransformActor.h"
#include "MikanShapeActor.h"
#include "MikanShapeTypes.h"
#include "MikanQuadShapeActor.generated.h"

UCLASS()
class UMikanQuadShapeData : public UMikanShapeData
{
	GENERATED_BODY()

public:
	UMikanQuadShapeData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline float GetQuadWidth() const { return QuadWidth; }
	inline float GetQuadHeight() const { return QuadHeight; }
	inline bool IsDoubleSided() const { return bIsDoubleSided; }

private:
	float QuadWidth = 0.f;
	float QuadHeight = 0.f;
	bool bIsDoubleSided = false;
};

UCLASS(BlueprintType, Blueprintable)
class AMikanQuadShapeActor : public AMikanShapeActor
{
	GENERATED_BODY()

public:
	AMikanQuadShapeActor(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
	virtual void BindMikanComponentData(UMikanComponentData* Data) override;

	UFUNCTION(BlueprintPure)
	FVector2D GetQuadSize() const { return QuadSize; }

private:
	FVector2D QuadSize;
};
