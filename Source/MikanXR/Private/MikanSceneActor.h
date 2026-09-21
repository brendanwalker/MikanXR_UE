#pragma once

#include "GameFramework/Actor.h"
#include "MikanTransformActor.h"
#include "MikanDataStore.h"
#include "MikanSceneActor.generated.h"

UCLASS()
class UMikanSceneData : public UMikanTransformData
{
	GENERATED_BODY()

public:
	UMikanSceneData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline int32 GetParentStageId() const { return ParentStageId; }
	inline int32 GetDisplayCompositorId() const { return DisplayCompositorId; }
	inline bool GetForceRender() const { return bForceRender; }

private:
	int32 ParentStageId = -1;
	int32 DisplayCompositorId = -1;
	bool bForceRender = false;
};

// The scene system's own values. Mirrors which scene the Mikan editor has active, so this
// client draws the same scene the editor's viewport and its compositors do.
UCLASS()
class UMikanSceneSystemData : public UMikanSystemData
{
	GENERATED_BODY()

public:
	UMikanSceneSystemData() = default;

	virtual void Initialize(const Serialization::PolymorphicObjectPtr& InValuesObject) override;
	virtual bool ApplyMikanValue(const FString& FieldName, const MikanVariant& FieldValue) override;
	virtual void Describe(TArray<FString>& OutLines) const override;

	inline int32 GetCurrentSceneId() const { return CurrentSceneId; }

private:
	int32 CurrentSceneId = -1;
};

UCLASS(BlueprintType, Blueprintable)
class MIKANXR_API AMikanSceneActor : public AMikanTransformActor
{
	GENERATED_BODY()

public:
	AMikanSceneActor(const FObjectInitializer& ObjectInitializer);

	virtual void PostRegisterAllComponents() override;
	virtual void Destroyed() override;

	// Scene Events
	void HandleSceneActivated();
	UFUNCTION(BlueprintImplementableEvent)
	void OnSceneActivated();
	void HandleSceneDeactivated();
	UFUNCTION(BlueprintImplementableEvent)
	void OnSceneDeactivated();

	// Script Message Events
	UFUNCTION()
	void HandleScriptMessage(const FString& Message);
	UFUNCTION(BlueprintImplementableEvent)
	void OnMikanScriptMessage(const FString& Message);

	inline UMikanSceneData* GetSceneData() const { return Cast<UMikanSceneData>(GetTransformData()); }

	// Hides or shows this scene and everything attached below it. A hidden primitive casts no
	// shadow, which is the point: an inactive scene's depth proxy must not shadow the live shot.
	void SetSceneSubtreeHidden(bool bNewHidden);

protected:
	virtual void OnComponentDataChanged(const FString& FieldName) override;
};
