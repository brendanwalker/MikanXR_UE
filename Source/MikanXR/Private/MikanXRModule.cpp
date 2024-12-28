// Copyright (c) 2023 Brendan Walker. All rights reserved.

#include "CoreMinimal.h"
#include "IMikanXRModule.h"
#include "Modules/ModuleInterface.h"
#include "Containers/Ticker.h"
#include "DynamicRHI.h"
#include "HAL/UnrealMemory.h"
#include "Logging/LogMacros.h"
#include "Misc/App.h"
#include "Misc/CString.h"
#include "Misc/Paths.h"
#include "Misc/EngineVersion.h"
#include "MikanAPI.h"
#include "MikanClientEvents.h"
#include "MikanClientRequests.h"
#include "MikanWorldSubsystem.h"

DEFINE_LOG_CATEGORY(MikanXRLog);

class FMikanXRModule : public IMikanXRModule
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	bool Tick(float DeltaTime);

	static void MIKAN_CALLBACK MikanLogCallback(int log_level, const char* log_message);

	FTickerDelegate TickDelegate;
	FTSTicker::FDelegateHandle TickDelegateHandle;
	float MikanReconnectTimeout= 0.f;
	bool bAutoReconnectEnabled= true;
	bool bIsConnected= false;

	// IMikanXRModule
	virtual void ConnectSubsystem(UMikanWorldSubsystem* World) override;
	virtual void DisconnectSubsystem(UMikanWorldSubsystem* World) override;
	virtual bool GetIsConnected() override;
	virtual IMikanAPI* GetMikanAPI() override
	{
		// Expose the raw C++ pointer to other unreal systems, rather than the smart pointer.
		// The MikanAPI's lifetime is owned by the module and will be destroyed when the module is unloaded,
		// an thus we don't want other modules to hold onto a reference to it.
		return MikanAPI.get();
	}
	virtual void GetClientInfo(MikanClientInfo* OutClientInfo) override
	{
		*OutClientInfo= ClientInfo;
	}

	IMikanAPIPtr MikanAPI= nullptr;
	MikanClientInfo ClientInfo;
	TArray<UMikanWorldSubsystem*> ConnectedSubsystems;
};

IMPLEMENT_MODULE(FMikanXRModule, MikanXR)

void FMikanXRModule::StartupModule()
{
	MikanAPI = IMikanAPI::createMikanAPI();
	if (!MikanAPI)
	{
		UE_LOG(MikanXRLog, Error, TEXT("Failed to create MikanXR API"));
		return;
	}

	MikanAPIResult Result= MikanAPI->init(MikanLogLevel_Info, &FMikanXRModule::MikanLogCallback);
	if (Result == MikanAPIResult::Success)
	{
		UE_LOG(MikanXRLog, Log, TEXT("Initialized MikanXR API"));

		const FString EngineVersion = FEngineVersion::Current().ToString();
		const auto EngineVersionAnsi = StringCast<ANSICHAR>(*EngineVersion);
		const auto ProjectNameAnsi = StringCast<ANSICHAR>(FApp::GetProjectName());
		const auto ProjectVerAnsi = StringCast<ANSICHAR>(FApp::GetBuildVersion());

		ClientInfo= MikanAPI->allocateClientInfo();
		ClientInfo.supportsRGBA32 = true;
		ClientInfo.supportsBGRA32= true;
		ClientInfo.supportsDepth= true;
		ClientInfo.engineName= "UnrealEngine";
		ClientInfo.engineVersion= EngineVersionAnsi.Get();
		ClientInfo.applicationName= ProjectNameAnsi.Get();
		ClientInfo.applicationVersion= ProjectVerAnsi.Get();
		ClientInfo.xrDeviceName= "";

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

		// Store the graphics device interface in the MikanAPI
		if (ClientInfo.graphicsAPI != MikanClientGraphicsApi_UNKNOWN)
		{
			void* graphicsDeviceInterface= GDynamicRHI->RHIGetNativeDevice();

			if (graphicsDeviceInterface != nullptr)
			{
				MikanAPI->setGraphicsDeviceInterface(
					ClientInfo.graphicsAPI, 
					graphicsDeviceInterface);
			}
		}
	}
	else
	{
		UE_LOG(MikanXRLog, Error, TEXT("Failed MikanXR API (error code: %d)"), Result);
	}
}

void FMikanXRModule::ShutdownModule()
{
	if (MikanAPI)
	{
		MikanAPI->shutdown();
		MikanAPI= nullptr;
	}
}

void FMikanXRModule::ConnectSubsystem(UMikanWorldSubsystem* Subsystem)
{
	if (MikanAPI)
	{
		void* graphicsDeviceInterface = nullptr;
		MikanAPI->getGraphicsDeviceInterface(
			ClientInfo.graphicsAPI,
			&graphicsDeviceInterface);

		check(graphicsDeviceInterface == GDynamicRHI->RHIGetNativeDevice());

		if (ConnectedSubsystems.Num() == 0)
		{
			TickDelegate = FTickerDelegate::CreateRaw(this, &FMikanXRModule::Tick);
			TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);
		}

		ConnectedSubsystems.AddUnique(Subsystem);
	}
}

void FMikanXRModule::DisconnectSubsystem(UMikanWorldSubsystem* Subsystem)
{
	ConnectedSubsystems.Remove(Subsystem);

	if (ConnectedSubsystems.Num() == 0)
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);

		if (MikanAPI->getIsConnected())
		{
			DisposeClientRequest disposeRequest = {};
			MikanAPI->sendRequest(disposeRequest).awaitResponse();

			MikanAPI->disconnect();
		}
	}
}

bool FMikanXRModule::GetIsConnected()
{
	return MikanAPI && MikanAPI->getIsConnected();
}

bool FMikanXRModule::Tick(float DeltaTime)
{
	if (MikanAPI)
	{
		if (MikanAPI->getIsConnected())
		{
			MikanEventPtr mikanEvent;
			while (MikanAPI->fetchNextEvent(mikanEvent) == MikanAPIResult::Success)
			{
				// Handle connection related events
				if (typeid(*mikanEvent) == typeid(MikanConnectedEvent))
				{
					// Send client info back to the server on connection
					InitClientRequest initClientRequest = {};
					initClientRequest.clientInfo = ClientInfo;

					MikanAPI->sendRequest(initClientRequest).awaitResponse();
				}
				else if (typeid(*mikanEvent) == typeid(MikanDisconnectedEvent))
				{
					auto disconnectEvent = std::static_pointer_cast<MikanDisconnectedEvent>(mikanEvent);
					const std::string reason = disconnectEvent->reason.getValue();

					if (disconnectEvent->code == MikanDisconnectCode_IncompatibleVersion)
					{
						// The server has disconnected us because we are using an incompatible version
						// Disable reconnection, since it is never going to work
						bAutoReconnectEnabled = true;
						UE_LOG(MikanXRLog, Error, TEXT("MikanDisconnectedEvent: Disable reconnect due to incompatible client"));
					}
					else
					{
						UE_LOG(MikanXRLog, Log, TEXT("MikanDisconnectedEvent: %s"), ANSI_TO_TCHAR(reason.c_str()));
					}
				}

				// Forward the event to all connected subsystems
				for (UMikanWorldSubsystem* Subsystem : ConnectedSubsystems)
				{
					Subsystem->HandleMikanEvent(mikanEvent);
				}
			}
		}
		else
		{
			if (ConnectedSubsystems.Num() > 0)
			{
				if (MikanReconnectTimeout <= 0.f)
				{
					if (MikanAPI->connect() != MikanAPIResult::Success)
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
	}

	return true;
}

void FMikanXRModule::MikanLogCallback(int log_level, const char* log_message)
{
	switch (log_level)
	{
		case MikanLogLevel_Trace:
			UE_LOG(MikanXRLog, VeryVerbose, TEXT("%s"), ANSI_TO_TCHAR(log_message));
			break;
		case MikanLogLevel_Debug:
			UE_LOG(MikanXRLog, Verbose, TEXT("%s"), ANSI_TO_TCHAR(log_message));
			break;
		case MikanLogLevel_Info:
			UE_LOG(MikanXRLog, Log, TEXT("%s"), ANSI_TO_TCHAR(log_message));
			break;
		case MikanLogLevel_Warning:
			UE_LOG(MikanXRLog, Warning, TEXT("%s"), ANSI_TO_TCHAR(log_message));
			break;
		case MikanLogLevel_Error:
			UE_LOG(MikanXRLog, Error, TEXT("%s"), ANSI_TO_TCHAR(log_message));
			break;
		case MikanLogLevel_Fatal:
			UE_LOG(MikanXRLog, Fatal, TEXT("%s"), ANSI_TO_TCHAR(log_message));
			break;
	}
}