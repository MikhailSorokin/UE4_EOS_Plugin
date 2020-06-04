// Copyright (C) 2019-2020, Gaslight Games Ltd


#include "CreateLobbyWidget.h"
#include <string>
#include <iostream>

int32 UCreateLobbyWidget::StringAsInt(FString InString)
{
	std::string stdstring = TCHAR_TO_UTF8(*InString);
	return (int32)std::stoi(stdstring);
}