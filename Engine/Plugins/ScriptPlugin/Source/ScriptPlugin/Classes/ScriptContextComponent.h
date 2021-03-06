// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ScriptBlueprintGeneratedClass.h"
#include "ScriptContextComponent.generated.h"

/** 
 * Script-extendable component class
 */
UCLASS(EarlyAccessPreview)
class SCRIPTPLUGIN_API UScriptContextComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:

	// Begin UActorComponent interface.
	virtual void OnRegister() override;
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void BeginDestroy() override;
	// Begin UActorComponent interface.

	/**
	* Calls a script-defined function (no arguments)
	* @param FunctionName Name of the function to call
	*/
	UFUNCTION(BlueprintCallable, Category = "ScriptContext")
	void CallScriptFunction(FString FunctionName);

protected:

	/** Script context (code) */
	FScriptContextBase* Context;
};

