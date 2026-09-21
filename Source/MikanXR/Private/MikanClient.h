#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MikanEngineSubsystem.h"
#include "MikanCameraEvents.h"
#include "GameFramework/Actor.h"

#include "MikanClient.generated.h"

USTRUCT()
struct FMikanCameraNewFrameEvent
{
	GENERATED_BODY()

	int32 CameraID= -1;
	FTransform CameraTransform;
	FMatrix ProjectionMatrix;
	float NearClippingPlaneUU= 0.f;
	float FarClippingPlaneUU= 0.f;
	float HorizontalFOVDegrees;
	int FrameWidth= 0;
	int FrameHeight= 0;
	int64_t FrameIndex= 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMikanNewFrameEvent, const FMikanCameraNewFrameEvent&, NewFrameEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMikanActiveSceneChangeEvent, class AMikanSceneActor*, OldScene, class AMikanSceneActor*, NewScene);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMikanRenderableEvent, class UMikanRenderableComponent*, Renderable);

USTRUCT()
struct FMikanSystemActors
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<int32, AMikanTransformActor*> SpawnedActorsTable;
};

UCLASS()
class MIKANXR_API AMikanClient : public AActor
{
	GENERATED_BODY()

public:

	AMikanClient(const FObjectInitializer& ObjectInitializer);

	virtual void PostRegisterAllComponents() override;
	virtual void Destroyed() override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void SetActiveMikanScene(class AMikanSceneActor* DesiredScene);

	UFUNCTION(BlueprintPure)
	inline class AMikanSceneActor* GetActiveMikanScene() const { return ActiveMikanScene; }

	// Points the active scene at whichever scene the Mikan editor has current. Blueprints can
	// still override the choice afterwards through SetActiveMikanScene.
	void RefreshActiveSceneFromEditor();

	// Hides every scene subtree but the active one, minus any scene with Force Render set. The
	// editor's project viewport applies the same rule, so both sides show the same geometry.
	void RefreshSceneVisibility();

	IMikanAPI* GetMikanAPI() const;
	const MikanClientInfo* GetClientInfo() const;

	bool RegisterMikanRenderable(class UMikanRenderableComponent* Renderable);
	void UnregisterMikanRenderable(class UMikanRenderableComponent* Renderable);

	UFUNCTION(BlueprintPure)
	inline TArray<class UMikanRenderableComponent*>& GetRegisteredMikanRenderables() { return RegisteredRenderables; }

	UFUNCTION(BlueprintPure)
	const class AMikanTransformActor* GetTransformActorByIdConst(int32 TransformID) const;
	UFUNCTION(BlueprintPure)
	class AMikanTransformActor* GetTransformActorById(int32 TransformID);

	inline const TMap<FString, FMikanSystemActors>& GetSystemActorTable() const { return SystemActorTable; }

	const FTransform* GetSavedStageTransform(int32 StageId) const;
	void SetSavedStageTransform(int32 StageId, const FTransform& Transform);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanAnchorActor> AnchorClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanQuadStencilActor> QuadStencilClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanBoxStencilActor> BoxStencilClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanModelStencilActor> ModelStencilClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanQuadShapeActor> QuadShapeClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanBoxShapeActor> BoxShapeClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanModelShapeActor> ModelShapeClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanCameraActor> CameraClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanSceneActor> SceneClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanStageActor> StageClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanRGBSpotLightActor> RGBSpotLightClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanRGBPixelGridActor> RGBPixelGridCameraClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mikan")
	TSubclassOf<class AMikanLightEnvironmentActor> LightEnvironmentClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* ModelStencilColorMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* ModelStencilDepthMaterial;
	// White, shadow-receiving catcher material used by the model-stencil shadow mesh. Authored
	// in-editor; the shadow capture renders this to produce the per-channel shadow (A/B) factor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* ModelStencilShadowMaterial;
	// Parent material for texturing a model stencil's placement mesh with its capture-frame
	// texture (the sibling .png of model_path, written by Mikan's depth mesh capture). Needs a
	// Texture parameter named "BaseTexture". Optional: when unset the placement mesh uses the
	// color material as before.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* ModelStencilTexturedPlacementMaterial;

	/**
	 * Divide material for the shadow A/B pass.
	 * Must expose two texture parameters named: "A"  and "B" (reference catcher)
	 * 
	 * "A": Shadowed catcher, lit with the character casting its shadow 
	 *      thus A = irradiance * shadowAttenuation
	 * "B": Unshadowed reference, catcher lit without the character
	 *      thus B = irradiance
	 * 
	 *  output = clamp((A + eps) / (B + eps), 0, 1)
	 *         = (irradiance * shadowAttenuation) / irradiance = shadowAttenuation
	 * 
	 * This material is drawn into the published shadow render target.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* ShadowDivideMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* ModelShapeColorMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UMaterialInterface* ModelShapeDepthMaterial;

	// Scene Events
	UPROPERTY(BlueprintAssignable)
	FMikanActiveSceneChangeEvent OnActiveSceneChanged;

	// Renderable Events
	UPROPERTY(BlueprintAssignable)
	FMikanRenderableEvent OnRenderableRegistered;
	UPROPERTY(BlueprintAssignable)
	FMikanRenderableEvent OnRenderableUnregistered;

	// Relayed events from UMikanEngineSubsystem (per-world subscribers bind here)
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnMikanConnected;
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnMikanDisconnected;
	UPROPERTY(BlueprintAssignable)
	FMikanSimpleEvent OnDMXDataChanged;

	UPROPERTY(BlueprintAssignable)
	FMikanScriptMessageEvent OnScriptMessage;

	// World-scaled camera frame event (positions converted from meters to Unreal Units)
	UPROPERTY(BlueprintAssignable)
	FMikanNewFrameEvent OnNewFrameEvent;

#if WITH_EDITOR
	// Outcome of the last editor-driven fetch. The details panel shows it so a fetch that never
	// gets its answers reads as a failure instead of as a button that did nothing.
	enum class EEditorFetchStatus : uint8
	{
		Idle,
		InProgress,
		Succeeded,
		Failed
	};

	void EditorRefetchFromMikan();
	void EditorClearMikanActors();

	inline EEditorFetchStatus GetEditorFetchStatus() const { return EditorFetchStatus; }
	inline const FString& GetEditorFetchMessage() const { return EditorFetchMessage; }
#endif
	
protected:
	void BindToEngineSubsystem();
	void UnbindFromEngineSubsystem();

	// Engine subsystem relay handlers
	UFUNCTION() 
	void HandleMikanConnected();
	UFUNCTION() 
	void HandleMikanDisconnected();
	UFUNCTION()
	void HandleDMXDataChanged();
	UFUNCTION() 
	void HandleScriptMessage(const FString& Message);
	void HandleCameraNewFrameRaw(const MikanCameraNewFrameEvent& NewFrameEvent);

	void SyncAllSpawnedActors();
	void HandleComponentListChanged(const UMikanComponentSystem* System);
	void HandleSystemDataChanged(const UMikanComponentSystem* System, const FString& FieldName);
	void SyncSystemSpawnedActors(const UMikanComponentSystem* System, bool bRefreshAttachments);
	void DespawnAllSpawnedActors();
	void RefreshAllSpawnedActorAttachments();
	UClass* GetMikanActorClassForSystem(const FString& SystemName) const;
	AMikanTransformActor* SpawnMikanActor(class UMikanTransformData* ComponentData);

	UPROPERTY()
	TMap<int32, FTransform> StageTransforms;

	UPROPERTY(Transient)
	class UMikanDataStore* DataStore = nullptr;

	// Table tracking all Mikan Transform Actors spawned by each system
	UPROPERTY(Transient)
	TMap<FString, FMikanSystemActors> SystemActorTable;

	UPROPERTY(Transient)
	class AMikanSceneActor* ActiveMikanScene = nullptr;
	UPROPERTY(Transient)
	TArray<class UMikanRenderableComponent*> RegisteredRenderables;

#if WITH_EDITOR
	EEditorFetchStatus EditorFetchStatus = EEditorFetchStatus::Idle;
	FString EditorFetchMessage;
#endif
};
