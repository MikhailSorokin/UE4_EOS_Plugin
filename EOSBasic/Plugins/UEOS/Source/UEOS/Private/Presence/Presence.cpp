// Copyright (C) Gaslight Games Ltd, 2019-2020

#pragma once

// UEOS Includes
#include "Presence/Presence.h"
#include "UEOSManager.h"
#include "UserInfo/UserInfo.h"

#include "UEOSModule.h"
#include <cassert>

//TODO - Class is in the works - will be used with friend interface - Mikhail 

UEOSPresence::UEOSPresence()
{

}

void UEOSPresence::QueryFriendPresence(const FBPCrossPlayInfo& InFriendInfo)
{
	EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(UEOSManager::GetPlatformHandle());
	FEpicAccountId AccountId = FEpicAccountId();
	EOS_Presence_QueryPresenceOptions Options;
	Options.ApiVersion = EOS_PRESENCE_QUERYPRESENCE_API_LATEST;
	Options.LocalUserId = UEOSManager::GetAuthentication()->GetEpicAccountId();
	Options.TargetUserId = AccountId.FromString(InFriendInfo.IdAsString);

	FBPCrossPlayInfo* NewFriendInfo = new FBPCrossPlayInfo(InFriendInfo);

	EOS_Presence_QueryPresence(PresenceHandle, &Options, NewFriendInfo, QueryPresenceCompleteCallback);
}

void UEOSPresence::QueryPresenceCompleteCallback(const EOS_Presence_QueryPresenceCallbackInfo* Data)
{
	assert(Data != nullptr);

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		UE_LOG(UEOSLog, Log, TEXT("[EOS SDK | Plugin] Error when getting presence status: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode));
		return;
	}

	UEOSPresence* EOSPresenceInfo = UEOSManager::GetPresence();

	FBPCrossPlayInfo* CrossPlayFriendInfo = (FBPCrossPlayInfo*)(Data->ClientData);

	FBPCrossPlayInfo NewFriend = UpdatePresenceStatus(*CrossPlayFriendInfo, Data->TargetUserId);

	GEngine->AddOnScreenDebugMessage(-1, 7.f, FColor::Yellow, FString::Printf(TEXT("User info: %s"), *NewFriend.DisplayName));

	GEngine->AddOnScreenDebugMessage(-1, 7.f, FColor::Yellow, FString::Printf(TEXT("Presence status: %d"), NewFriend.Presence));

	if (EOSPresenceInfo->OnFriendPresenceQueryComplete.IsBound()) {
		EOSPresenceInfo->OnFriendPresenceQueryComplete.Broadcast(NewFriend);
	}
	delete CrossPlayFriendInfo;
}

FBPCrossPlayInfo UEOSPresence::UpdatePresenceStatus(FBPCrossPlayInfo& InFriendInfo, FEpicAccountId TargetId)
{
	EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(UEOSManager::GetPlatformHandle());

	EOS_Presence_CopyPresenceOptions CopyOptions;
	CopyOptions.ApiVersion = EOS_PRESENCE_COPYPRESENCE_API_LATEST;
	CopyOptions.LocalUserId = UEOSManager::GetAuthentication()->GetEpicAccountId();
	CopyOptions.TargetUserId = TargetId;

	EOS_Presence_Info* PresenceData = nullptr;
	EOS_EResult ResultCode = EOS_Presence_CopyPresence(PresenceHandle, &CopyOptions, &PresenceData);

	if (ResultCode != EOS_EResult::EOS_Success)
	{
		UE_LOG(UEOSLog, Warning, TEXT("[EOS SDK | Plugin] Error when getting presence status: %s"), *UEOSCommon::EOSResultToString(ResultCode));
		return FBPCrossPlayInfo();
	}


	FName Platform = FName(*FString(UTF8_TO_TCHAR(PresenceData->Platform)));
	GEngine->AddOnScreenDebugMessage(-1, 7.f, FColor::Cyan, Platform.ToString());

	if (Platform == FName("Windows"))
			InFriendInfo.PlatformType = EPlatformType::Epic;
	else if (Platform == FName("Steam"))
			InFriendInfo.PlatformType = EPlatformType::Steam;
		//So on and so forth..

	uint8 Why = static_cast<uint8>(PresenceData->Status);
	InFriendInfo.Presence = static_cast<EPresenceStatus>(Why);

	switch (PresenceData->Status)
	{
	case EOS_Presence_EStatus::EOS_PS_Online:
		InFriendInfo.PresenceString = "ONLINE";
		break;
	case EOS_Presence_EStatus::EOS_PS_Offline:
		InFriendInfo.PresenceString = "OFFLINE";
		break;
	}

	GEngine->AddOnScreenDebugMessage(-1, 7.f, FColor::Cyan, FString::Printf(TEXT("Presence status: %d"), PresenceData->Status));

	//TODO - Figure out what these are and if there are useful to presence data
	//InFriendInfo.Application = FString(UTF8_TO_TCHAR(PresenceData->ProductId));
	//PresenceInfo.RichText = FStringUtils::Widen(PresenceInfo->RichText);

	EOS_Presence_Info_Release(PresenceData);

	return InFriendInfo;
}



void UEOSPresence::SetPresence(FEpicAccountId TargetUserId)
{
	EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(UEOSManager::GetPlatformHandle());
	EOS_Presence_SetPresenceOptions Options;
	Options.ApiVersion = EOS_PRESENCE_SETPRESENCE_API_LATEST;
	Options.LocalUserId = UEOSManager::GetAuthentication()->GetEpicAccountId();

	EOS_HPresenceModification PresenceModificationHandle;
	EOS_Presence_CreatePresenceModificationOptions PresenceModificationOptions;
	PresenceModificationOptions.LocalUserId = UEOSManager::GetAuthentication()->GetEpicAccountId();
	PresenceModificationOptions.ApiVersion = EOS_PRESENCE_CREATEPRESENCEMODIFICATION_API_LATEST;
	EOS_EResult EResult = EOS_Presence_CreatePresenceModification(PresenceHandle, &PresenceModificationOptions, &PresenceModificationHandle);

	if (EResult == EOS_EResult::EOS_Success)
	{
		Options.PresenceModificationHandle = PresenceModificationHandle;

		EOS_Presence_SetPresence(PresenceHandle, &Options, TargetUserId, SetPresenceCallback);

		EOS_PresenceModification_Release(PresenceModificationHandle);

	} else
	{
		UE_LOG(UEOSLog, Log, TEXT("%s: threw error of: %s"), __FUNCTIONW__, *UEOSCommon::EOSResultToString(EResult));
	}

}

void UEOSPresence::SetPresenceCallback(const EOS_Presence_SetPresenceCallbackInfo* Data)
{
	check(Data != nullptr);

	UE_LOG(UEOSLog, Log, TEXT("On set presence result code: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode));

	
}


void UEOSPresence::SubscribeToFriendPresenceUpdates()
{
	EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(UEOSManager::GetPlatformHandle());
	EOS_Presence_AddNotifyOnPresenceChangedOptions Options;
	Options.ApiVersion = EOS_PRESENCE_ADDNOTIFYONPRESENCECHANGED_API_LATEST;
	EOS_NotificationId PresenceNotificationId = EOS_Presence_AddNotifyOnPresenceChanged(PresenceHandle, &Options, NULL, OnPresenceChangedCallback);

	if (!PresenceNotificationId)
	{
		UE_LOG(UEOSLog, Warning, TEXT("[EOS SDK]: could not subscribe to presence updates."));
	}
	/*else
	{
		PresenceNotifications[UserId] = PresenceNotificationId;
	}*/
}

void UEOSPresence::OnPresenceChangedCallback(const EOS_Presence_PresenceChangedCallbackInfo* Data)
{
	if (Data != nullptr) {
		UEOSUserInfo* EOSUserInfo = UEOSManager::GetUserInfo();

		EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(UEOSManager::GetPlatformHandle());
		FEpicAccountId AccountId = FEpicAccountId();
		EOS_Presence_QueryPresenceOptions Options;
		Options.ApiVersion = EOS_PRESENCE_QUERYPRESENCE_API_LATEST;
		Options.LocalUserId = UEOSManager::GetAuthentication()->GetEpicAccountId();
		Options.TargetUserId = Data->PresenceUserId;

		FEpicAccountId PresenceUserInfo = FEpicAccountId(Data->PresenceUserId);
		FBPCrossPlayInfo CrossPlayInfo = FBPCrossPlayInfo();
		CrossPlayInfo.IdAsString = PresenceUserInfo.ToString();
		CrossPlayInfo.DisplayName = EOSUserInfo->GetDisplayName(PresenceUserInfo);

		FBPCrossPlayInfo* NewFriendInfo = new FBPCrossPlayInfo(CrossPlayInfo);

		EOS_Presence_QueryPresence(PresenceHandle, &Options, NewFriendInfo, QueryPresenceCompleteCallback);
	}
	else
	{
		UE_LOG(UEOSLog, Warning, TEXT("[EOS SDK | Plugin] Error when querying presence info for udpates."));
		
	}
}


