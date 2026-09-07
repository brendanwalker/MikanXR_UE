#pragma once

#include "CoreMinimal.h"
#include "MikanAPI.h"

#include "MikanDMXDataStore.generated.h"

UCLASS()
class UMikanDMXUniverse : public UObject
{
	GENERATED_BODY()

public:
	UMikanDMXUniverse() = default;

	static constexpr int32 kDMXUniverseChannelCount = 512;

	void Initialize(int32 UniverseId);
	void ApplyMikanUniverseData(const struct MikanUniverseDMXData& UniverseData);
	uint8_t ReadChannelValue(const int32 Channel) const;
	inline int32 GetUniverseId() const { return UniverseId; }

private:
	int32 UniverseId;
	uint8_t ChannelData[kDMXUniverseChannelCount]{};
};

UCLASS()
class UMikanDMXDataStore : public UObject
{
	GENERATED_BODY()

public:
	UMikanDMXDataStore()= default;

	void Initialize();
	void ApplyMikanUniverseData(const struct MikanLightDMXDataChangedEvent& DataChangeEvent);

	UMikanDMXUniverse* GetDMXUniverse(int32 UniverseId);
	inline const TMap<int32, UMikanDMXUniverse*>& GetDMXUniverses() const { return DMXUniverses; }
	inline double GetLastUpdateServerTime() const { return LastUpdateServerTime; }

protected:
	UMikanDMXUniverse* FindOrAddDMXUniverse(int32 UniverseId);

private:
	double LastUpdateServerTime = 0;

	UPROPERTY(Transient)
	TMap<int32, UMikanDMXUniverse*> DMXUniverses;
};
