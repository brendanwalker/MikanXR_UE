// Copyright (c) 2023 Brendan Walker. All rights reserved.

#include "CoreMinimal.h"
#include "IMikanXRModule.h"
#include "Modules/ModuleInterface.h"
#include "Containers/Ticker.h"
#include "DynamicRHI.h"
#include "HAL/UnrealMemory.h"
#include "Misc/App.h"
#include "Misc/CString.h"
#include "Misc/Paths.h"
#include "Misc/EngineVersion.h"
#include "MikanClient_CAPI.h"
#include "MikanWorldSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(MikanXR, Log, All);
DEFINE_LOG_CATEGORY(MikanXR);

class FMikanXRModule : public IMikanXRModule
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	bool Tick(float DeltaTime);

	static void MIKAN_CALLBACK MikanLogCallback(int log_level, const char* log_message);

	FTickerDelegate TickDelegate;
	FDelegateHandle TickDelegateHandle;
	float MikanReconnectTimeout= 0.f;
	bool bAutoReconnectEnabled= true;
	bool bIsConnected= false;

	// IMikanXRModule
	virtual void ConnectSubsystem(UMikanWorldSubsystem* World) override;
	virtual void DisconnectSubsystem(UMikanWorldSubsystem* World) override;
	virtual bool GetIsConnected() override;
	virtual void GetClientInfo(MikanClientInfo* OutClientInfo)
	{
		*OutClientInfo= ClientInfo;
	}

	MikanClientInfo ClientInfo;
	TArray<UMikanWorldSubsystem*> ConnectedSubsystems;
};

IMPLEMENT_MODULE(FMikanXRModule, MikanXR)

void FMikanXRModule::StartupModule()
{
	MikanResult Result= Mikan_Initialize(MikanLogLevel_Info, &FMikanXRModule::MikanLogCallback);
	if (Result == MikanResult_Success)
	{
		UE_LOG(MikanXR, Log, TEXT("Initialized MikanXR API"));

		const FString EngineVersion = FEngineVersion::Current().ToString();
		const TCHAR* ProjectName = FApp::GetProjectName();
		const TCHAR* ProjectVer = FApp::GetBuildVersion();

		FMemory::Memset(&ClientInfo, 0, sizeof(MikanClientInfo));
		ClientInfo.supportedFeatures = MikanFeature_RenderTarget_BGRA32;
		FCStringAnsi::Strncpy(ClientInfo.engineName, "UnrealEngine", sizeof(ClientInfo.engineName) - 1);
		FCStringAnsi::Strncpy(ClientInfo.engineVersion, TCHAR_TO_ANSI(*EngineVersion), sizeof(ClientInfo.engineVersion) - 1);
		FCStringAnsi::Strncpy(ClientInfo.applicationName, TCHAR_TO_ANSI(ProjectName), sizeof(ClientInfo.applicationName) - 1);
		FCStringAnsi::Strncpy(ClientInfo.applicationVersion, TCHAR_TO_ANSI(ProjectVer), sizeof(ClientInfo.applicationVersion) - 1);
		ClientInfo.xrDeviceName[0] = '\0';
		FCStringAnsi::Strncpy(ClientInfo.mikanSdkVersion, Mikan_GetVersionString(), sizeof(ClientInfo.mikanSdkVersion) - 1);

		const FString RHIName= GDynamicRHI->GetName();
		if (RHIName == TEXT("D3D9"))
		{
			ClientInfo.graphicsAPI = MikanClientGraphicsApi_Direct3D9;
		}
		else if (RHIName == TEXT("D3D11"))
		{
			ClientInfo.graphicsAPI = MikanClientGraphicsApi_Direct3D11;
		}
		else if (RHIName == TEXT("D3D12"))
		{
			ClientInfo.graphicsAPI = MikanClientGraphicsApi_Direct3D12;
		}
		else if (RHIName == TEXT("OpenGL"))
		{
			ClientInfo.graphicsAPI = MikanClientGraphicsApi_OpenGL;
		}
		else
		{
			ClientInfo.graphicsAPI = MikanClientGraphicsApi_UNKNOWN;
		}

		if (ClientInfo.graphicsAPI != MikanClientGraphicsApi_UNKNOWN)
		{
			void* graphicsDeviceInterface= GDynamicRHI->RHIGetNativeDevice();

			if (graphicsDeviceInterface != nullptr)
			{
				Mikan_SetGraphicsDeviceInterface(ClientInfo.graphicsAPI, graphicsDeviceInterface);
			}
		}
	}
	else
	{
		UE_LOG(MikanXR, Error, TEXT("Failed MikanXR API (error code: %d)"), Result);
	}
}

void FMikanXRModule::ShutdownModule()
{
	Mikan_Shutdown();
}

void FMikanXRModule::ConnectSubsystem(UMikanWorldSubsystem* Subsystem)
{
	void* graphicsDeviceInterface= nullptr;
	Mikan_GetGraphicsDeviceInterface(ClientInfo.graphicsAPI, &graphicsDeviceInterface);
	check(graphicsDeviceInterface == GDynamicRHI->RHIGetNativeDevice());

	if (ConnectedSubsystems.Num() == 0)
	{
		TickDelegate = FTickerDelegate::CreateRaw(this, &FMikanXRModule::Tick);
		TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
	}

	ConnectedSubsystems.AddUnique(Subsystem);
}

void FMikanXRModule::DisconnectSubsystem(UMikanWorldSubsystem* Subsystem)
{
	ConnectedSubsystems.Remove(Subsystem);

	if (ConnectedSubsystems.Num() == 0)
	{
		FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);

		if (Mikan_GetIsConnected())
		{
			Mikan_Disconnect();
		}
	}
}

bool FMikanXRModule::GetIsConnected()
{
	return Mikan_GetIsConnected();
}

bool FMikanXRModule::Tick(float DeltaTime)
{
	if (Mikan_GetIsConnected())
	{
		MikanEvent event;
		while (Mikan_PollNextEvent(&event) == MikanResult_Success)
		{
			for (UMikanWorldSubsystem* Subsystem : ConnectedSubsystems)
			{
				Subsystem->HandleMikanEvent(&event);
			}
		}
	}
	else 
	{
		if (ConnectedSubsystems.Num() > 0)
		{
			if (MikanReconnectTimeout <= 0.f)
			{
				if (Mikan_Connect(&ClientInfo) != MikanResult_Success)
				{
					// timeout between reconnect attempts
					MikanReconnectTimeout = 1.0f;
				}
			}
			else
			{
				MikanReconnectTimeout -= DeltaTime;
			}
		}
	}

	return true;
}

void FMikanXRModule::MikanLogCallback(int log_level, const char* log_message)
{
	switch (log_level)
	{
		case MikanLogLevel_Trace:
			UE_LOG(MikanXR, VeryVerbose, TEXT("%s"), ANSI_TO_TCHAR(log_message));
			break;
		case MikanLogLevel_Debug:
			UE_LOG(MikanXR, Verbose, TEXT("%s"), ANSI_TO_TCHAR(log_message));
			break;
		case MikanLogLevel_Info:
			UE_LOG(MikanXR, Log, TEXT("%s"), ANSI_TO_TCHAR(log_message));
			break;
		case MikanLogLevel_Warning:
			UE_LOG(MikanXR, Warning, TEXT("%s"), ANSI_TO_TCHAR(log_message));
			break;
		case MikanLogLevel_Error:
			UE_LOG(MikanXR, Error, TEXT("%s"), ANSI_TO_TCHAR(log_message));
			break;
		case MikanLogLevel_Fatal:
			UE_LOG(MikanXR, Fatal, TEXT("%s"), ANSI_TO_TCHAR(log_message));
			break;
	}
}
