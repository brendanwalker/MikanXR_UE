#pragma once

#include "GameFramework/Actor.h"
#include "MikanDataStore.h"
#include "MikanTransformActor.h"
#include "MikanStencilActor.generated.h"

UCLASS()
class UMikanStencilData : public UMikanTransformData
{
	GENERATED_BODY()

public:
	UMikanStencilData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline bool IsDisabled() const { return bIsDisabled; }
	inline int32 GetCullMode() const { return CullMode; }

private:
	bool bIsDisabled = false;
	int32 CullMode = 0;
};

UCLASS(BlueprintType)
class AMikanStencilActor : public AMikanTransformActor
{
	GENERATED_BODY()
public:
};
