// Copyright (C) Gaslight Games Ltd, 2019

#pragma once

// Engine Includes
#include "Object.h"

// EOS Includes
#include "eos_sdk.h"
#include "eos_logging.h"

#include "UEOSManager.generated.h"

// Forward Declarations


UCLASS()
class UEOS_API UEOSManager : public UObject
{
	GENERATED_BODY()

public:

	/**
	* EOS Manager Constructor.
	*/
	UEOSManager();

	/**
	* Use this class as a Singleton and thus, returns the current instance.
	* @return UEOSManager The singleton active instance.
	*/
	UFUNCTION( BlueprintCallable, Category = "UEOS|Manager" )
		static UEOSManager*						GetEOSManager();

	/**
	* Cleans up all content within the Singleton.
	* Requests a delete of the current UEOSManager instance.
	*/
	UFUNCTION( BlueprintCallable, Category = "UEOS|Manager" )
		static void								Cleanup();

protected:

	UFUNCTION( BlueprintCallable, Category = "UEOS|Manager" )
		bool									InitEOS();

	UFUNCTION( BlueprintCallable, Category = "UEOS|Manager" )
		bool									ShutdownEOS();

	// --------------------------------------------------------------
	// STATIC PROPERTIES
	// --------------------------------------------------------------

	/** The singleton UEOSManager instance. */
	static UEOSManager*							EOSManager;

	// --------------------------------------------------------------
	// INSTANCE PROPERTIES
	// --------------------------------------------------------------

	EOS_HPlatform								PlatformHandle;

private:

	/**
	* Callback function to use for EOS SDK log messages
	*
	* @param InMsg - A structure representing data for a log message
	*/
	static void									EOSSDKLoggingCallback( const EOS_LogMessage* InMsg );
};
