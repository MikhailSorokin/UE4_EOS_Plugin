
// Copyright (C) Gaslight Games Ltd, 2019

#pragma once

// Engine Includes
#include "UObject/Object.h"

// EOS Includes
#include "eos_sdk.h"
#include "eos_connect.h"
#include "eos_version.h"
#include "eos_common.h"
#include "UObject/CoreOnline.h"
#include "Connect.generated.h"

//TODO: The ContinuanceToken should be added as a parameter to this delegate; however, it needs an "Unreal-friendly" type to hold it.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnectLoginSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnectLoginFail);

UENUM()
enum class EExternalCredentialType : uint8
{
	/* Epic Games Account */
	ECT_Epic = 0	UMETA(DisplayName = "Epic"),

	/* Steam Encrypted App Ticket*/
	ECT_Steam_App_Ticket = 1	UMETA(DisplayName = "Steam")
};

UCLASS()
class UEOS_API UEOSConnect : public UObject
{
	GENERATED_BODY()

public:

	/**
	* EOS Connect Constructor.
	*/
	UEOSConnect();

	FString PlayerDisplayName;
	EOS_ContinuanceToken GlobalContinuanceToken;

	/**
	 * In EpicGames - this is just the display name from the UserInfo class
	 * In Steam, this is the display name from the subsystem
	 *
	 * @param InPlayerDisplayName - name of player as a string
	 */
	UFUNCTION(BlueprintCallable)
		void InitializeParameters(FString InPlayerDisplayName);
	
	const FString EnumToString(const TCHAR* Enum, int32 EnumValue);

	UFUNCTION(BlueprintCallable)
		/**
		* Attempts to login to the EOS Connect interface using the specified type of credentials (also known as: Identity Provider).
		*
		* @param ExternalCredentialType
		*/
		void Login(EExternalCredentialType ExternalCredentialType);

	void CreateUser(const EOS_ContinuanceToken* InContinuanceToken);

	/** Fires when a Login has completed successfully. 
	* According to the documentation (https://dev.epicgames.com/docs/services/en-US/Interfaces/Connect/index.html), when this occurs, the player should be prompted to login with another identity provider, potentially linking their account with an ID they have used before.
	*/
	UPROPERTY(BlueprintAssignable, Category = "UEOS|Connect")
		FOnConnectLoginSuccess OnLoginSuccess;

	/** Fires when a Login has failed.
	*/
	UPROPERTY(BlueprintAssignable, Category = "UEOS|Connect")
		FOnConnectLoginFail OnLoginFail;
	//FOnEncryptedAppTicketResponseDelegate ResSteamResult;

	
protected:

	static void CreateUserCompleteCallback(const EOS_Connect_CreateUserCallbackInfo* Data);
	static void OnLinkAccountCallback(const EOS_Connect_LinkAccountCallbackInfo* Data);

	
	static void LoginUserCompleteCallback(const EOS_Connect_LoginCallbackInfo* Data);
	
	/** Handle for Connect interface. */
	EOS_HConnect						ConnectHandle;

protected:
	bool bCallOnce;

private:

	void HandleSteamEncryptedAppTicketResponse(bool bEncryptedDataAvailable, int32 ResultCode);

	/* Helper function that calls the SDK's EOS_Connect_Login function. */
	static void DoEOSConnectLogin(const char* CredentialsToken, EOS_EExternalCredentialType CredentialsType, const EOS_Connect_UserLoginInfo* UserLoginInfo = nullptr);
	
	//static FOnlineEncryptedAppTicketSteamPtr GetSteamEncryptedAppTicketInterface();
};
