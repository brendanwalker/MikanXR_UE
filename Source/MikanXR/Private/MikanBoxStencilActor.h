#pragma once

#include "GameFramework/Actor.h"
#include "MikanTransformActor.h"
#include "MikanStencilActor.h"
#include "MikanStencilTypes.h"
#include "MikanBoxStencilActor.generated.h"

UCLASS()
class UMikanBoxStencilData : public UMikanStencilData
{
	GENERATED_BODY()

public:
	UMikanBoxStencilData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline float GetBoxXSize() const { return BoxXSize; }
	inline float GetBoxYSize() const { return BoxYSize; }
	inline float GetBoxZSize() const { return BoxZSize; }

private:
	float BoxXSize = 0.f;
	float BoxYSize = 0.f;
	float BoxZSize = 0.f;
};

UCLASS(BlueprintType, Blueprintable)
class AMikanBoxStencilActor : public AMikanStencilActor
{
	GENERATED_BODY()

public:
	AMikanBoxStencilActor(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	virtual void BindMikanComponentData(class UMikanComponentData* Data) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Extents;
};
