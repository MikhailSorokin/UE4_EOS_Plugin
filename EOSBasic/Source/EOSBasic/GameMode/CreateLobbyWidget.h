// Copyright (C) 2019-2020, Gaslight Games Ltd

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "CreateLobbyWidget.generated.h"

/**
 * 
 */
UCLASS()
class EOSBASIC_API UCreateLobbyWidget : public UUserWidget
{
	GENERATED_BODY()
	
	public:
		UFUNCTION(BlueprintCallable, BlueprintPure)
			int32 StringAsInt(FString InString);
};
