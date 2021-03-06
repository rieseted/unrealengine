// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Helper struct defining parameters we need to establish a connection */
struct FOneSkyConnectionInfo
{
	/** Name of this connection configuration */
	FString Name;

	/** OneSky ApiKey */
	FString ApiKey;

	/** OneSky ApiSecret */
	FString ApiSecret;
};