// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/Lobby.h"
#include "UEOSModule.h"
#include "UEOSManager.h"
#include "Connect/Connect.h"
#include "Authentication/Authentication.h"

/* ================================= Blueprint function calls =========================== */

//NOTE: Can only be used if Connect interface is called after authentication login
void UEOSLobby::CreateLobby(int32 InLobbyMembers)
{
	if (!UEOSManager::GetConnect()->bAuthorized)
	{
		UE_LOG(UEOSLog, Error, TEXT("%s: You have not connected to your EAS account! Unknown how to authorize."), __FUNCTIONW__);
	}

	UE_LOG(UEOSLog, Log, TEXT("%s: lobby being created."), __FUNCTIONW__);
	
	EOS_Lobby_CreateLobbyOptions Options = EOS_Lobby_CreateLobbyOptions();
	Options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	Options.LocalUserId = UEOSManager::GetConnect()->GetProductId();
	Options.MaxLobbyMembers = InLobbyMembers;
	Options.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());
	EOS_Lobby_CreateLobby(LobbyHandle, &Options, this, CallBackLobbyTest);
}


void UEOSLobby::FindLobby(int32 InMaxSearchResults)
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

	EOS_Lobby_CreateLobbySearchOptions Options = EOS_Lobby_CreateLobbySearchOptions();
	Options.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
	Options.MaxResults = InMaxSearchResults;

	EOS_HLobbySearch* OutLobbySearchHandle = nullptr;

	EOS_EResult Result = EOS_Lobby_CreateLobbySearch(LobbyHandle, &Options, OutLobbySearchHandle);
	if (Result == EOS_EResult::EOS_Success)
	{
		LobbySearchHandle = *OutLobbySearchHandle;

		OnSearchResultsReceived();

		//JoinLobby();
	}
	else
	{
		FString ErrorText = FString::Printf(TEXT("[EOS SDK | Lobby] Lobby search Failed - Error Code: %s"), *UEOSCommon::EOSResultToString(Result));
		UE_LOG(UEOSLog, Error, TEXT("%s"), *ErrorText);

		if (OnLobbySearchFailed.IsBound()) {
			OnLobbySearchFailed.Broadcast();
		}
	}

}


/* ===================================== Callbacks ============================ */

void UEOSLobby::CallBackLobbyTest(const EOS_Lobby_CreateLobbyCallbackInfo* Data)
{
	check(Data != nullptr);

	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UEOSManager::GetLobby()->CurrentLobbyId = Data->LobbyId;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, (TEXT("Lobby succeeded: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (TEXT("Lobby failed: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
}

void UEOSLobby::JoinLobbyCallback(const EOS_Lobby_JoinLobbyCallbackInfo* Data)
{
	check(Data != nullptr);

	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UEOSManager::GetLobby()->CurrentLobbyId = Data->LobbyId;
		//EOS_LobbyDetails_Info* LobbyInfo;

		UEOSManager::GetLobby()->OnSearchResultsReceived();
		
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, (TEXT("Lobby succeeded: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (TEXT("Lobby failed: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
}


void UEOSLobby::OnSearchResultsReceived()
{
	EOS_LobbySearch_GetSearchResultCountOptions SearchResultOptions = {};
	SearchResultOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
	uint32_t NumSearchResults = EOS_LobbySearch_GetSearchResultCount(UEOSManager::GetLobby()->LobbySearchHandle, &SearchResultOptions);

	TArray<FBPLobbySearchResult> SearchResults;
	//TArray<LobbyDetailsKeeper> ResultHandles;

	EOS_LobbySearch_CopySearchResultByIndexOptions IndexOptions = {};
	IndexOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
	for (uint32_t SearchIndex = 0; SearchIndex < NumSearchResults; ++SearchIndex)
	{
		FBPLobbySearchResult NextLobby;

		EOS_HLobbyDetails NextLobbyDetails = nullptr;
		IndexOptions.LobbyIndex = SearchIndex;
		EOS_EResult Result = EOS_LobbySearch_CopySearchResultByIndex(UEOSManager::GetLobby()->LobbySearchHandle, &IndexOptions, &NextLobbyDetails);
		if (Result == EOS_EResult::EOS_Success && NextLobbyDetails)
		{

			//get owner
			EOS_LobbyDetails_GetLobbyOwnerOptions GetOwnerOptions = {};
			GetOwnerOptions.ApiVersion = EOS_LOBBYDETAILS_GETLOBBYOWNER_API_LATEST;
			FEpicProductId NewLobbyOwner = FEpicProductId(EOS_LobbyDetails_GetLobbyOwner(NextLobbyDetails, &GetOwnerOptions));

			//copy lobby info
			EOS_LobbyDetails_CopyInfoOptions CopyInfoDetails;
			CopyInfoDetails.ApiVersion = EOS_LOBBYDETAILS_COPYINFO_API_LATEST;
			EOS_LobbyDetails_Info* LobbyInfo = nullptr;
			Result = EOS_LobbyDetails_CopyInfo(NextLobbyDetails, &CopyInfoDetails, &LobbyInfo);
			if (Result != EOS_EResult::EOS_Success || !LobbyInfo)
			{
				continue;
			}

			NextLobby.LobbyId = LobbyInfo->LobbyId;
			NextLobby.OwnerId = NewLobbyOwner;
			NextLobby.OwnerIdString = NextLobby.OwnerId.ToString();
			
			SearchResults.Add(NextLobby);
		}
	}

	//Search is done, we can release this memory
	EOS_LobbySearch_Release(UEOSManager::GetLobby()->LobbySearchHandle);

	//TODO - For now , we choose a random integer session
	int32 RandomIndex = FMath::RandRange(0, NumSearchResults - 1);
	if (UEOSManager::GetLobby()->OnLobbySearchSucceeded.IsBound()) {
		UEOSManager::GetLobby()->OnLobbySearchSucceeded.Broadcast(SearchResults[RandomIndex]);
	}
}


/*
EOS_HLobbyDetails DetailsHandle;

EOS_LOBBYI

EOS_Lobby_CopyLobbyDetailsHandleOptions CopyHandleOptions = {};
CopyHandleOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
CopyHandleOptions.LobbyId = ;
CopyHandleOptions.LocalUserId = UEOSManager::GetConnect()->GetProductId();

EOS_HLobbyDetails LobbyDetailsHandle = nullptr;
Result = EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &CopyHandleOptions, &LobbyDetailsHandle);

EOS_LobbySearch_(UEOSManager::GetLobby()->LobbySearchHandle, &IndexOptions, &NextLobbyDetails);

//Found a lobby, let us join a random one (if possible)
EOS_Lobby_JoinLobbyOptions Options;
Options.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
Options.LobbyDetailsHandle = DetailsHandle;
Options.LocalUserId = UEOSManager::GetConnect()->GetProductId();


EOS_Lobby_JoinLobby(LobbyHandle, &Options, nullptr, JoinLobbyCallback);
*/