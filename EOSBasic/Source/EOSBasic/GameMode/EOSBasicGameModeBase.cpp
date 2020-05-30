// Fill out your copyright notice in the Description page of Project Settings.

#include "EOSBasicGameModeBase.h"

#include "Authentication/Authentication.h"
#include "UEOSManager.h"

#include "eos_lobby.h"
/*#include "../Plugins/Online/OnlineSubsystemSteam/Source/Private/IPAddressSteam.h"
#include "Online.h"
#include "OnlineEngineInterface.h"
#include "../Plugins/Online/OnlineSubsystemSteam/Source/Private/OnlineSessionInterfaceSteam.h"
#include "OnlineSessionSettings.h"
#include "../Plugins/Online/OnlineSubsystemSteam/Source/Private/OnlineSubsystemSteamTypes.h"
#include "../Plugins/Online/OnlineSubsystemSteam/Source/Public/OnlineEncryptedAppTicketInterfaceSteam.h"*/

#include "OnlineEncryptedAppTicketInterfaceSteam.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"

#include "steam/steam_api.h"

// TODO - Put in Lobby folder
/*
void AEOSBasicGameModeBase::LobbyTest()
{
	EOS_Lobby_CreateLobbyOptions Options = EOS_Lobby_CreateLobbyOptions();
	Options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	Options.LocalUserId = this->LocalUserId;
	Options.MaxLobbyMembers = 4;
	Options.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;

	this->LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());
	EOS_Lobby_CreateLobby(this->LobbyHandle, &Options, this, CallBackLobbyTest);
}

void AEOSBasicGameModeBase::CallBackLobbyTest(const EOS_Lobby_CreateLobbyCallbackInfo* Data)
{
	check(Data != nullptr);

	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, (TEXT("Lobby succeeded: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (TEXT("Lobby failed: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
}
*/