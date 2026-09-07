#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "MikanDebugLibrary.generated.h"

// Inspection and control entry points for driving the plugin from outside the process through the
// Remote Control API. Every function is static so the library's object path is fixed
// (/Script/MikanXR.Default__MikanDebugLibrary) and a client needs no discovery step to start.
//
// Replies are text lines, one "<field> <value>" or one record per line. A failure is a single
// line beginning "error:". Actor queries run against the PIE world while a session is running,
// otherwise the editor world, otherwise the game world.
UCLASS()
class UMikanDebugLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// -- Connection to the Mikan editor -----
	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> GetConnectionStatus();

	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> SetWantsToBeConnected(bool bConnect);

	// Round trips to the Mikan editor. These block on the reply, so they answer an error when not connected.
	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> GetAppStage();

	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> PushAppStage(const FString& StageName);

	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> PopAppStage();

	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> SendRemoteControlCommand(const FString& CommandType, const TArray<FString>& CommandArgs);

	// -- Cached component data store -----
	// "<systemName> <componentClass> <componentCount>" per system
	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> ListSystems();

	// "<componentId> <componentName>" per cached component of the system
	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> ListComponents(const FString& SystemName);

	// "<field> <value>" per cached field, keyed by the Mikan property name
	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> DescribeComponent(const FString& SystemName, int32 ComponentId);

	// -- Cached DMX universes -----
	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> ListDMXUniverses();

	// "<channel> <value>" for every non-zero channel of the universe
	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> DescribeDMXUniverse(int32 UniverseId);

	// -- Actors spawned by the Mikan client in the resolved world -----
	// Object path of the AMikanClient actor, usable as the objectPath of a Remote Control property or call request
	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> GetMikanClientPath();

	// "<systemName> <transformId> <parentTransformId> <actorPath>" per spawned actor
	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> ListSpawnedActors();

	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> DescribeActor(int32 TransformId);

	// The details panel's Fetch From Mikan and Clear Editor Actors buttons. Editor only.
	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> RefetchFromMikan();

	// Outcome of the last editor fetch, the same state the details panel shows. Editor only.
	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> GetFetchStatus();

	UFUNCTION(BlueprintCallable, Category = "Mikan|Debug")
	static TArray<FString> ClearMikanActors();

private:
	static UWorld* ResolveDebugWorld();
	static class UMikanEngineSubsystem* FindEngineSubsystem(TArray<FString>& OutLines);
	static class UMikanDataStore* FindDataStore(TArray<FString>& OutLines);
	static class UMikanComponentSystem* FindComponentSystem(const FString& SystemName, TArray<FString>& OutLines);
	static class AMikanClient* FindMikanClient(TArray<FString>& OutLines);
	static void AppendTransformLines(TArray<FString>& OutLines, const TCHAR* Prefix, const FTransform& Transform);
};
