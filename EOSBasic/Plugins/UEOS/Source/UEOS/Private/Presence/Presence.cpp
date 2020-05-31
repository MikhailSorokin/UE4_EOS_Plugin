// Copyright (C) Gaslight Games Ltd, 2019-2020

#pragma once

// UEOS Includes
#include "Presence/Presence.h"
#include "UEOSManager.h"
#include "UserInfo/UserInfo.h"

#include "UEOSModule.h"
#include <cassert>
#include "Friends/Friends.h"

// Class is used heavily with the friend interface - Mikhail 

UEOSPresence::UEOSPresence()
{

}

//NOTE: Should be called once Friends with names are set
void UEOSPresence::QueryFriendsPresenceInfo()
{
	TArray<FBPCrossPlayInfo> AllFriends = UEOSManager::GetFriends()->TempCrossPlayFriends;
	FEpicAccountId LocalEpicAccount = UEOSManager::GetAuthentication()->GetEpicAccountId();
	
	for (const FBPCrossPlayInfo& Friend : AllFriends)
	{
		//NOTE: We can only request presence info on our confirmed friends, not friends who are pending or have just gotten removed
		if (Friend.FriendshipStatus == EBPFriendStatus::Friends)
		{
			QueryPresenceInfo(LocalEpicAccount, Friend.AccountId);
		}
	}

}

void UEOSPresence::QueryPresenceInfo(FEpicAccountId LocalAccount, FEpicAccountId FriendAccount)
{
	EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(UEOSManager::GetPlatformHandle());
	FEpicAccountId AccountId = FEpicAccountId();
	EOS_Presence_QueryPresenceOptions Options;
	Options.ApiVersion = EOS_PRESENCE_QUERYPRESENCE_API_LATEST;

	Options.LocalUserId = LocalAccount;
	Options.TargetUserId = FriendAccount;

	EOS_Presence_QueryPresence(PresenceHandle, &Options, nullptr, QueryUserPresenceCompleteCallback);
}

void UEOSPresence::QueryUserPresenceCompleteCallback(const EOS_Presence_QueryPresenceCallbackInfo* Data)
{
	assert(Data != nullptr);

	if (Data->ResultCode != EOS_EResult::EOS_Success)
	{
		UE_LOG(UEOSLog, Log, TEXT("[EOS SDK | Plugin] Error when getting presence status: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode));
		return;
	}

	UEOSPresence* EOSPresenceInfo = UEOSManager::GetPresence();
	FBPPresenceInfo UpdatedPresenceInfo = UpdatePresenceStatus(Data->LocalUserId, Data->TargetUserId);

	TArray<FBPCrossPlayInfo> Friends = UEOSManager::GetFriends()->TempCrossPlayFriends;
	for (FBPCrossPlayInfo Friend : Friends)
	{
		if (Friend.AccountId == FEpicAccountId(Data->TargetUserId)) {
			Friend.Presence = UpdatedPresenceInfo;

			// Last callback  needed in the friend initial gathering sequence
			if (EOSPresenceInfo->OnFriendPresenceQueryComplete.IsBound()) {
				EOSPresenceInfo->OnFriendPresenceQueryComplete.Broadcast(Friend);
			}

			return;
		}
	}
}

FBPPresenceInfo UEOSPresence::UpdatePresenceStatus(FEpicAccountId LocalId, FEpicAccountId FriendId)
{
	EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(UEOSManager::GetPlatformHandle());

	EOS_Presence_CopyPresenceOptions CopyOptions;
	CopyOptions.ApiVersion = EOS_PRESENCE_COPYPRESENCE_API_LATEST;
	CopyOptions.LocalUserId = LocalId;
	CopyOptions.TargetUserId = FriendId;

	EOS_Presence_Info* PresenceData = nullptr;
	EOS_EResult ResultCode = EOS_Presence_CopyPresence(PresenceHandle, &CopyOptions, &PresenceData);

	if (ResultCode != EOS_EResult::EOS_Success)
	{
		UE_LOG(UEOSLog, Warning, TEXT("[EOS SDK | Plugin] Error when getting presence status: %s"), *UEOSCommon::EOSResultToString(ResultCode));
		return FBPPresenceInfo();
	}

	FBPPresenceInfo PresenceInfo;

	//TODO - See all kinds of platforms that are available;
	PresenceInfo.Platform = FString(UTF8_TO_TCHAR(PresenceData->Platform));

	GEngine->AddOnScreenDebugMessage(-1, 7.f, FColor::Cyan, PresenceInfo.Platform);

	PresenceInfo.Presence = (EPresenceStatus)(PresenceData->Status);

	GEngine->AddOnScreenDebugMessage(-1, 7.f, FColor::Cyan, FString::Printf(TEXT("Presence status: %d"), PresenceInfo.Presence));

	PresenceInfo.Application = FString(UTF8_TO_TCHAR(PresenceData->ProductId));
	PresenceInfo.RichText = FString(UTF8_TO_TCHAR(PresenceData->RichText));

	EOS_Presence_Info_Release(PresenceData);

	return PresenceInfo;
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

	UE_LOG(UEOSLog, Log, TEXT("On set presence result code: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode));

	
}


void UEOSPresence::SubscribeToFriendPresenceUpdates()
{
	FEpicAccountId AccountId = UEOSManager::GetEOSManager()->GetAuthentication()->GetEpicAccountId();
	
	EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(UEOSManager::GetPlatformHandle());
	EOS_Presence_AddNotifyOnPresenceChangedOptions Options;
	Options.ApiVersion = EOS_PRESENCE_ADDNOTIFYONPRESENCECHANGED_API_LATEST;
	EOS_NotificationId NotificationId = EOS_Presence_AddNotifyOnPresenceChanged(PresenceHandle, &Options, NULL, OnPresenceChangedCallback);

	//We add the presence status in a map for every local user that we have
	if (!NotificationId)
	{
		UE_LOG(UEOSLog, Warning, TEXT("[EOS SDK]: could not subscribe to presence updates."));
	}
	else
	{
		PresenceNotifications.Add(AccountId, NotificationId);
	}
}

void UEOSPresence::UnsubscribeFromFriendPresenceUpdates(FEpicAccountId UserId)
{
	EOS_HPresence PresenceHandle = EOS_Platform_GetPresenceInterface(UEOSManager::GetPlatformHandle());

	if (PresenceNotifications.Contains(UserId)) {
		EOS_Presence_RemoveNotifyOnPresenceChanged(PresenceHandle, PresenceNotifications[UserId]);
		PresenceNotifications.Remove(UserId);
	}
}

void UEOSPresence::OnPresenceChangedCallback(const EOS_Presence_PresenceChangedCallbackInfo* Data)
{
	if (Data != nullptr) {
		UEOSManager::GetPresence()->QueryPresenceInfo(Data->LocalUserId, Data->PresenceUserId);
	}
	else
	{
		UE_LOG(UEOSLog, Warning, TEXT("[EOS SDK | Plugin] Error when querying presence info for udpates."));
		
	}
}


