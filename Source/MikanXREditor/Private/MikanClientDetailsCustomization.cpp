#include "MikanClientDetailsCustomization.h"
#include "MikanClient.h"
#include "MikanEngineSubsystem.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MikanXREditor"

namespace
{
	// The row lambdas outlive the customize call, so resolve the actor each time rather than
	// capturing it: the selection can change or the actor can be destroyed underneath the panel.
	AMikanClient* FindCustomizedClient(const TArray<TWeakObjectPtr<UObject>>& Objects)
	{
		for (const TWeakObjectPtr<UObject>& ObjPtr : Objects)
		{
			if (AMikanClient* Client = Cast<AMikanClient>(ObjPtr.Get()))
			{
				return Client;
			}
		}

		return nullptr;
	}
}

TSharedRef<IDetailCustomization> FMikanClientDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FMikanClientDetailsCustomization);
}

void FMikanClientDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	IDetailCategoryBuilder& MikanCategory = DetailBuilder.EditCategory(
		"Mikan",
		LOCTEXT("MikanCategory", "Mikan"),
		ECategoryPriority::Important);

	// Connection status row
	MikanCategory.AddCustomRow(LOCTEXT("ConnectionStatus", "Connection Status"))
	.WholeRowContent()
	[
		SNew(STextBlock)
		.Text_Lambda([]()
		{
			UMikanEngineSubsystem* ES = UMikanEngineSubsystem::Get();
			if (ES)
			{
				if (ES->GetIsConnected())
				{
					if (ES->GetWantsToBeConnected())
						return LOCTEXT("Connected", "Mikan: Connected");
					else
						return LOCTEXT("Disconnecting", "Mikan: Disconnecting...");
				}
				else 
				{
					if (ES->GetWantsToBeConnected())
						return LOCTEXT("Connecting", "Mikan: Connecting...");
					else
						return LOCTEXT("Disconnected", "Mikan: Disconnected");
				}
			}
			else
			{
				return LOCTEXT("Unloaded", "Mikan: Unloaded");
			}
		})
		.ColorAndOpacity_Lambda([]()
		{
			UMikanEngineSubsystem* ES = UMikanEngineSubsystem::Get();
			bool bConnected = ES && ES->GetIsConnected();
			return bConnected ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor(1.f, 0.5f, 0.f));
		})
	];

	// Last fetch status row
	MikanCategory.AddCustomRow(LOCTEXT("FetchStatusRow", "Fetch Status"))
	.WholeRowContent()
	[
		SNew(STextBlock)
		.Text_Lambda([Objects]()
		{
			AMikanClient* Client = FindCustomizedClient(Objects);
			if (!Client)
			{
				return FText::GetEmpty();
			}

			switch (Client->GetEditorFetchStatus())
			{
			case AMikanClient::EEditorFetchStatus::InProgress:
				return LOCTEXT("FetchInProgress", "Fetch: in progress...");
			case AMikanClient::EEditorFetchStatus::Succeeded:
				return FText::FromString(FString::Printf(
					TEXT("Fetch: %s"), *Client->GetEditorFetchMessage()));
			case AMikanClient::EEditorFetchStatus::Failed:
				return FText::FromString(FString::Printf(
					TEXT("Fetch failed: %s"), *Client->GetEditorFetchMessage()));
			default:
				return LOCTEXT("FetchIdle", "Fetch: no actors fetched");
			}
		})
		.ColorAndOpacity_Lambda([Objects]()
		{
			AMikanClient* Client = FindCustomizedClient(Objects);
			const AMikanClient::EEditorFetchStatus Status =
				Client ? Client->GetEditorFetchStatus() : AMikanClient::EEditorFetchStatus::Idle;

			switch (Status)
			{
			case AMikanClient::EEditorFetchStatus::Succeeded:
				return FSlateColor(FLinearColor::Green);
			case AMikanClient::EEditorFetchStatus::Failed:
				return FSlateColor(FLinearColor::Red);
			case AMikanClient::EEditorFetchStatus::InProgress:
				return FSlateColor(FLinearColor(1.f, 0.5f, 0.f));
			default:
				return FSlateColor::UseForeground();
			}
		})
	];

	// "Connect" button
	MikanCategory.AddCustomRow(LOCTEXT("ConnectRow", "Connect"))
		.WholeRowContent()
		[
			SNew(SButton)
				.Text(LOCTEXT("ConnectButton", "Connect"))
				.ToolTipText(LOCTEXT("ConnectTooltip", "Try connecting to Mikan."))
				.IsEnabled_Lambda([]()
					{
						UMikanEngineSubsystem* ES = UMikanEngineSubsystem::Get();
						return ES && !ES->GetWantsToBeConnected();
					})
				.OnClicked_Lambda([Objects]()
					{
						UMikanEngineSubsystem* ES = UMikanEngineSubsystem::Get();
						if (ES)
						{
							ES->SetWantsToBeConnected(true);
						}
						return FReply::Handled();
					})
		];

	// "Disconnect" button
	MikanCategory.AddCustomRow(LOCTEXT("DisconnectRow", "Disconnect"))
		.WholeRowContent()
		[
			SNew(SButton)
				.Text(LOCTEXT("DisconnectButton", "Disconnect"))
				.ToolTipText(LOCTEXT("DisconnectTooltip", "Try disconnecting from Mikan."))
				.IsEnabled_Lambda([]()
					{
						UMikanEngineSubsystem* ES = UMikanEngineSubsystem::Get();
						return ES && ES->GetWantsToBeConnected();
					})
				.OnClicked_Lambda([Objects]()
					{
						UMikanEngineSubsystem* ES = UMikanEngineSubsystem::Get();
						if (ES)
						{
							ES->SetWantsToBeConnected(false);
						}
						return FReply::Handled();
					})
		];

	// "Fetch From Mikan" button
	MikanCategory.AddCustomRow(LOCTEXT("FetchRow", "Refetch Mikan Actors"))
	.WholeRowContent()
	[
		SNew(SButton)
		.Text(LOCTEXT("FetchButton", "Fetch From Mikan"))
		.ToolTipText(LOCTEXT("FetchTooltip", "Connect to Mikan and spawn camera and light actors into the editor world."))
		.IsEnabled_Lambda([]()
		{
			UMikanEngineSubsystem* ES = UMikanEngineSubsystem::Get();
			return ES && ES->GetIsConnected();
		})
		.OnClicked_Lambda([Objects]()
		{
			for (const TWeakObjectPtr<UObject>& ObjPtr : Objects)
			{
				if (AMikanClient* Client = Cast<AMikanClient>(ObjPtr.Get()))
				{
					Client->EditorRefetchFromMikan();
				}
			}
			return FReply::Handled();
		})
	];

	// "Clear Editor Actors" button
	MikanCategory.AddCustomRow(LOCTEXT("ClearRow", "Clear Mikan Actors"))
	.WholeRowContent()
	[
		SNew(SButton)
		.Text(LOCTEXT("ClearButton", "Clear Editor Actors"))
		.ToolTipText(LOCTEXT("ClearTooltip", "Remove all camera and light actors that were fetched from Mikan."))
		.OnClicked_Lambda([Objects]()
		{
			for (const TWeakObjectPtr<UObject>& ObjPtr : Objects)
			{
				if (AMikanClient* Client = Cast<AMikanClient>(ObjPtr.Get()))
				{
					Client->EditorClearMikanActors();
				}
			}
			return FReply::Handled();
		})
	];
}

#undef LOCTEXT_NAMESPACE
