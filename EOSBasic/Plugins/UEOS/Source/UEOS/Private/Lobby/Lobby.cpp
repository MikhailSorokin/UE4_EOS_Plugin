// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/Lobby.h"
#include "UEOSModule.h"
#include "UEOSManager.h"
#include "Connect/Connect.h"

//NOTE: Can only be used if Connect interface is authorized
void UEOSLobby::CreateLobby(int32 InLobbyMembers)
{
	if (!UEOSManager::GetConnect()->bAuthorized)
	{
		UE_LOG(UEOSLog, Error, TEXT("%s: You have not connected to your EAS account! Unknown how to authorize."), __FUNCTIONW__);
	}
	
	EOS_Lobby_CreateLobbyOptions Options = EOS_Lobby_CreateLobbyOptions();
	Options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	Options.LocalUserId = UEOSManager::GetConnect()->GetProductId();
	Options.MaxLobbyMembers = InLobbyMembers;
	Options.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());
	EOS_Lobby_CreateLobby(LobbyHandle, &Options, this, CallBackLobbyTest);
}

void UEOSLobby::CallBackLobbyTest(const EOS_Lobby_CreateLobbyCallbackInfo* Data)
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