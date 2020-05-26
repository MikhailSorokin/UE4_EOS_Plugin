// Copyright (C) Gaslight Games Ltd, 2019-2020
/* Edited by Mikhail Sorokin 2020 */

#include "Connect/Connect.h"

#include "UEOSModule.h"
#include "UEOSManager.h"

#include <string>
#include "Engine/DemoNetConnection.h"
#include "Authentication/Authentication.h"

#include "UserInfo/UserInfo.h"

#include "OnlineSubsystemSteam.h"
#include "OnlineEncryptedAppTicketInterfaceSteam.h"
#include "OnlineAuthInterfaceUtilsSteam.h"


//TODO - Work in progress
UEOSConnect::UEOSConnect()
	: ConnectHandle( NULL )

{

}


void UEOSConnect::InitializeParameters(FString InPlayerDisplayName)
{
	PlayerDisplayName = InPlayerDisplayName;
}


void UEOSConnect::Login(EExternalCredentialType ExternalCredentialType)
{
	ConnectHandle = EOS_Platform_GetConnectInterface(UEOSManager::GetEOSManager()->GetPlatformHandle());

	// How to generate "Credentials.Token" varies by login method.
	// For Epic Games: get the required token Token from the EOS_Auth interface
	if (ExternalCredentialType == EExternalCredentialType::ECT_Epic)
	{
		UEOSAuthentication* Authentication = UEOSManager::GetEOSManager()->GetAuthentication();
		EOS_Auth_Token* UserAuthToken = nullptr;

		UE_LOG(UEOSLog, Display, TEXT("%s : Attempting connect login using Epic Games."), __FUNCTIONW__)

		bool GetAuthTokenCopySuccess = Authentication->GetAuthTokenCopy(&UserAuthToken);
		if (!GetAuthTokenCopySuccess)
		{
			UE_LOG(UEOSLog, Fatal, TEXT("%s : Failed to retrieve UserAuthToken."), __FUNCTIONW__);
			return;
		}

		/*EOS_Connect_UserLoginInfo LoginInfo;
		LoginInfo.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
		LoginInfo.DisplayName = TCHAR_TO_UTF8(*PlayerDisplayName);*/
		
		UEOSUserInfo* UserInfo = UEOSManager::GetUserInfo();
		FEpicAccountId EpicAccountId = UserAuthToken->AccountId;
		FString AuthDisplayName = UserInfo->GetDisplayName(EpicAccountId);
		UE_LOG(UEOSLog, Log, TEXT("%s : Access token value: %hs, name is: %s, type is: %d"), __FUNCTIONW__, UserAuthToken->AccessToken, *AuthDisplayName, static_cast<uint8>(UserAuthToken->AuthType));

		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, *FString::Printf(TEXT("%s :Access token value : %hs, name is : %s, type is : %d"), __FUNCTIONW__, UserAuthToken->AccessToken, *AuthDisplayName, static_cast<uint8>(UserAuthToken->AuthType)));
		DoEOSConnectLogin(UserAuthToken->AccessToken, EOS_EExternalCredentialType::EOS_ECT_EPIC);

		Authentication->ReleaseAuthToken(UserAuthToken);
	}
	// For Steam: generated using the ISteamUser::RequestEncryptedAppTicket API of Steamworks SDK, as described in eos_connect_types.h. We obtain from the Steam OnlineSubsystem.
	else if (ExternalCredentialType == EExternalCredentialType::ECT_Steam_App_Ticket) 
	{
		UE_LOG(UEOSLog, Display, TEXT("%s: Attempting connect login using Steam."), __FUNCTIONW__)

		FOnlineEncryptedAppTicketSteamPtr SteamEncryptedAppTicketInterface = GetSteamEncryptedAppTicketInterface();
		SteamEncryptedAppTicketInterface->OnEncryptedAppTicketResultDelegate.BindUObject(this, &ThisClass::HandleSteamEncryptedAppTicketResponse);

		bool bRequestQueued = SteamEncryptedAppTicketInterface->RequestEncryptedAppTicket(NULL, 0);
		if (!bRequestQueued)
		{
			// Login cannot proceed; request to Steam API was throttled. Cannot bind to the callback either, because we do not know what *data* is being encrypted by the call currently in progress.
			UE_LOG(UEOSLog, Fatal, TEXT("%s: RequestEncryptedAppTicket was not queued (already in progress)."), __FUNCTIONW__)
		}

		UE_LOG(UEOSLog, Verbose, TEXT("%s: Bound HandleSteamEncryptedAppTicketResponse to OnEncryptedAppTicketResultDelegate."), __FUNCTIONW__)
		// Connect login will continue when this callback occurs.
		
	}
	else
	{
		UE_LOG(UEOSLog, Fatal, TEXT("%s: Unhandled EExternalCredentialType: %d"), __FUNCTIONW__, ExternalCredentialType)
	}
}

void UEOSConnect::HandleSteamEncryptedAppTicketResponse(bool bEncryptedDataAvailable, int32 ResultCode)
{
	TArray<uint8> EncryptedBytes;

	UE_LOG(UEOSLog, Display, TEXT("%s. bEncryptedDataAvailable: %d. ResultCode: %d"), __FUNCTIONW__, bEncryptedDataAvailable, ResultCode)

	if (bEncryptedDataAvailable)
	{
		UE_LOG(UEOSLog, Verbose, TEXT("%s: Getting EncryptedAppTicket"), __FUNCTIONW__);
		bool bSuccess = GetSteamEncryptedAppTicketInterface()->GetEncryptedAppTicket(EncryptedBytes);
		if (bSuccess)
		{
			const uint32_t MaxEncodedDataLen = 2048;
			char* OutBuffer = new char[MaxEncodedDataLen];
			uint32_t OutEncodedDataLen = MaxEncodedDataLen;
			EOS_EResult Result = EOS_ByteArray_ToString(EncryptedBytes.GetData(), EncryptedBytes.Num(), OutBuffer, &OutEncodedDataLen);

			if (Result == EOS_EResult::EOS_Success) {
				UE_LOG(UEOSLog, Verbose, TEXT("%s: result is: %s"), __FUNCTIONW__, *EnumToString(TEXT("EOS_EResult"), static_cast<uint8>(Result)));
			}

			DoEOSConnectLogin(OutBuffer, EOS_EExternalCredentialType::EOS_ECT_STEAM_APP_TICKET, NULL);

			delete[] OutBuffer;

		}
		else
		{
			UE_LOG(UEOSLog, Fatal, TEXT("%s: GetEncryptedAppTicket failed."), __FUNCTIONW__)
		}
	}
	else
	{
		UE_LOG(UEOSLog, Fatal, TEXT("%s: The encrypted data is not available."), __FUNCTIONW__)
	}
}

void UEOSConnect::DoEOSConnectLogin(const char* CredentialsToken, EOS_EExternalCredentialType CredentialsType, const EOS_Connect_UserLoginInfo* UserLoginInfo)
{
	EOS_HConnect ConnectHandle = EOS_Platform_GetConnectInterface(UEOSManager::GetEOSManager()->GetPlatformHandle());
	
	EOS_Connect_Credentials Credentials;
	Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
	Credentials.Token = CredentialsToken;
	Credentials.Type = CredentialsType;

	//NOTE: The loginInfo throws an error with any device provider besides Apple&Nintendo, so keep NULL for others
	EOS_Connect_LoginOptions Options;
	Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
	Options.Credentials = &Credentials;
	Options.UserLoginInfo = UserLoginInfo;

	EOS_Connect_Login(ConnectHandle, &Options, (EOS_ContinuanceToken*)CredentialsToken, LoginUserCompleteCallback);
}


void UEOSConnect::LoginUserCompleteCallback(const EOS_Connect_LoginCallbackInfo* Data)
{
	UE_LOG(UEOSLog, Verbose, TEXT("%s: Calling EOS_Connect_Login. Options.Credentials.Token = %p"), __FUNCTIONW__, Data->ContinuanceToken);

	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, *FString::Printf(TEXT("%s: Calling EOS_Connect_Login. Options.Credentials.Token = %p"), __FUNCTIONW__, Data->ContinuanceToken));
	
	EOS_HConnect ConnectHandle = EOS_Platform_GetConnectInterface(UEOSManager::GetEOSManager()->GetPlatformHandle());

	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{

		EOS_ELoginStatus LoginStatus = EOS_Connect_GetLoginStatus(ConnectHandle, Data->LocalUserId);

		UE_LOG(UEOSLog, Log, TEXT("%s : login status %s"), __FUNCTIONW__, *UEOSManager::GetConnect()->EnumToString(TEXT("EOS_ELoginStatus"), static_cast<uint8>(LoginStatus)));

		//UE_LOG(UEOSLog, Log, TEXT("%s : user product id: %s"), __FUNCTIONW__, *FProductId(Data->LocalUserId).ToString());

		/**
		 * If the user was not found with credentials passed into EOS_Connect_Login,
		 * this continuance token can be passed to either EOS_Connect_CreateUser
		 * or EOS_Connect_LinkAccount to continue the flow
		 */
		UEOSManager::GetConnect()->CreateUser(&Data->ContinuanceToken);

		if (UEOSManager::GetConnect()->OnConnectLoginSuccess.IsBound())
		{
			UEOSManager::GetConnect()->OnConnectLoginSuccess.Broadcast();
		}
	}
	else
	{
		// Login failed due to error.
		UE_LOG(UEOSLog, Error, TEXT("%s: Connect login failed with result %s"), __FUNCTIONW__, *UEOSCommon::EOSResultToString(Data->ResultCode))

		if (UEOSManager::GetConnect()->OnConnectLoginFail.IsBound())
		{
			UEOSManager::GetConnect()->OnConnectLoginFail.Broadcast();
		}
	}
}


void UEOSConnect::CreateUser(const EOS_ContinuanceToken* InContinuanceToken)
{
	EOS_Connect_CreateUserOptions Options;
	Options.ContinuanceToken = *InContinuanceToken;
	GlobalContinuanceToken = Options.ContinuanceToken;
	Options.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
	EOS_Connect_CreateUser(ConnectHandle, &Options, NULL, CreateUserCompleteCallback);
}

void UEOSConnect::CreateUserCompleteCallback(const EOS_Connect_CreateUserCallbackInfo* Data)
{
	if (Data != NULL) {
		EOS_HConnect ConnectHandle = EOS_Platform_GetConnectInterface(UEOSManager::GetEOSManager()->GetPlatformHandle());

		if (Data->ResultCode != EOS_EResult::EOS_Success) {
			UE_LOG(UEOSLog, Warning, TEXT("%s : %s"), __FUNCTIONW__, *UEOSCommon::EOSResultToString(Data->ResultCode));
			return;
		}

		EOS_Connect_LinkAccountOptions Options;
		Options.LocalUserId = Data->LocalUserId;
		UEOSConnect* ConnectInterface = UEOSManager::GetEOSManager()->GetConnect();
		//NOTE: Create User does not pass in Continuance token from Login... have to store globally
		Options.ContinuanceToken = ConnectInterface->GlobalContinuanceToken;
		Options.ApiVersion = EOS_CONNECT_LINKACCOUNT_API_LATEST;
		EOS_Connect_LinkAccount(ConnectHandle, &Options, NULL, OnLinkAccountCallback);
	}
}

void UEOSConnect::OnLinkAccountCallback(const EOS_Connect_LinkAccountCallbackInfo* Data)
{
	UE_LOG(UEOSLog, Log, TEXT("%s : %s"), __FUNCTIONW__, *UEOSCommon::EOSResultToString(Data->ResultCode));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "Successfully linked your account to Epic Account Services!");
}

// Static function
FOnlineEncryptedAppTicketSteamPtr UEOSConnect::GetSteamEncryptedAppTicketInterface()
{
	FOnlineSubsystemSteam* SteamSubsystem = (FOnlineSubsystemSteam*) (IOnlineSubsystem::Get(STEAM_SUBSYSTEM));
	if (SteamSubsystem == nullptr)
	{
		UE_LOG(UEOSLog, Fatal, TEXT("%s: Failed to get Steam Subsystem!"), __FUNCTIONW__)
	}

	FOnlineEncryptedAppTicketSteamPtr SteamEncryptedAppTicketInterface = SteamSubsystem->GetEncryptedAppTicketInterface();
	if (!SteamEncryptedAppTicketInterface.IsValid())
	{
		UE_LOG(UEOSLog, Fatal, TEXT("%s: Failed to get encrypted app ticket interface of Steam subsystem."), __FUNCTIONW__)
	}

	return SteamEncryptedAppTicketInterface;
}

const FString UEOSConnect::EnumToString(const TCHAR* Enum, int32 EnumValue)
{
	const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, Enum, true);
	if (!EnumPtr)
		return NSLOCTEXT("Invalid", "Invalid", "Invalid").ToString();
	return EnumPtr->GetDisplayNameTextByIndex(EnumValue).ToString();
}
