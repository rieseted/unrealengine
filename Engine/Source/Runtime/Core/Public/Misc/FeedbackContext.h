// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UniquePtr.h"


struct FSlowTask;


/** A stack of feedback scopes */
struct FSlowTaskStack : TArray<FSlowTask*>
{
	/** Get the fraction of work that has been completed for the specified index in the stack (0=total progress) */
	CORE_API float GetProgressFraction(int32 Index) const;
};


/** A context for displaying modal warning messages. */
class CORE_API FFeedbackContext
	: public FOutputDevice
{
public:

	/** Ask the user a binary question, returning their answer */
	virtual bool YesNof( const FText& Question ) { return false; }
	
	/** Return whether the user has requested to cancel the current slow task */
	virtual bool ReceivedUserCancel() { return false; };
	
	/** Public const access to the current state of the scope stack */
	FORCEINLINE const FSlowTaskStack& GetScopeStack() const
	{
		return *ScopeStack;
	}

	/**** Legacy API - not deprecated as it's still in heavy use, but superceded by FScopedSlowTask ****/
	void BeginSlowTask( const FText& Task, bool ShowProgressDialog, bool bShowCancelButton=false );
	void UpdateProgress( int32 Numerator, int32 Denominator );
	void StatusUpdate( int32 Numerator, int32 Denominator, const FText& StatusText );
	void StatusForceUpdate( int32 Numerator, int32 Denominator, const FText& StatusText );
	void EndSlowTask();
	/**** end legacy API ****/

protected:

	/**
	 * Called to create a slow task
	 */
	virtual void StartSlowTask( const FText& Task, bool bShowCancelButton=false )
	{
		GIsSlowTask = true;
	}

	/**
	 * Called to destroy a slow task
	 */
	virtual void FinalizeSlowTask( )
	{
		GIsSlowTask = false;
	}

	/**
	 * Called when some progress has occurred
	 * @param	TotalProgressInterp		[0..1] Value indicating the total progress of the slow task
	 * @param	DisplayMessage			The message to display on the slow task
	 */
	virtual void ProgressReported( const float TotalProgressInterp, FText DisplayMessage ) {}

public:
	virtual FContextSupplier* GetContext() const { return NULL; }
	virtual void SetContext( FContextSupplier* InSupplier ) {}

	/** Shows/Closes Special Build Progress dialogs */
	virtual TWeakPtr<class SBuildProgressWidget> ShowBuildProgressWindow() {return TWeakPtr<class SBuildProgressWidget>();}
	virtual void CloseBuildProgressWindow() {}

	TArray<FString> Warnings;
	TArray<FString> Errors;

	bool	TreatWarningsAsErrors;

	FFeedbackContext()
		: TreatWarningsAsErrors(0)
		, ScopeStack(MakeShareable(new FSlowTaskStack))
	{}

	virtual ~FFeedbackContext();

private:
	FFeedbackContext(const FFeedbackContext&);
	FFeedbackContext& operator=(const FFeedbackContext&);

protected:
	
	friend FSlowTask;

	/** Stack of pointers to feedback scopes that are currently open */
	TSharedRef<FSlowTaskStack> ScopeStack;
	TArray<TUniquePtr<FSlowTask>> LegacyAPIScopes;

	/** Ask that the UI be updated as a result of the scope stack changing */
	void RequestUpdateUI(bool bForceUpdate = false);

	/** Update the UI as a result of the scope stack changing */
	void UpdateUI();

};

/** Enum to specify a particular slow task section should be shown */
enum class ESlowTaskVisibility
{
	/** Default visibility (inferred by some heuristic of remaining work/time open) */
	Default,
	/** Force this particular slow task to be visible on the UI */
	ForceVisible,
	/** Forcibly prevent this slow task from being shown, but still use it for work progress calculations */
	Invisible,
};

/**
 * Data type used to store information about a currently running slow task. Direct use is not advised, use FScopedSlowTask instead
 */
struct CORE_API FSlowTask
{
	/** Default message to display to the user when not overridden by a frame */
	FText DefaultMessage;

	/** Message pertaining to the current frame's work */
	FText FrameMessage;

	/** The amount of work to do in this scope */
	float TotalAmountOfWork;

	/** The amount of work we have already completed in this scope */
	float CompletedWork;

	/** The amount of work the current frame is responsible for */
	float CurrentFrameScope;

	/** The visibility of this slow task */
	ESlowTaskVisibility Visibility;

	/** The time that this scope was created */
	double StartTime;

private:

	/** Boolean flag to control whether this scope actually does anything (unset for quiet operations etc) */
	bool bEnabled;

	/** Flag that specifies whether this feedback scope created a new slow task dialog */
	bool bCreatedDialog;
	
	/** The feedback context that we belong to */
	FFeedbackContext& Context;

	/** Prevent copying */
	FSlowTask(const FSlowTask&);

public:

	/**
	 * Construct this scope from an amount of work to do, and a message to display
	 * @param		InAmountOfWork			Arbitrary number of work units to perform (can be a percentage or number of steps).
	 *										0 indicates that no progress frames are to be entered in this scope (automatically enters a frame encompassing the entire scope)
	 * @param		InDefaultMessage		A message to display to the user to describe the purpose of the scope
	 * @param		bInVisible				When false, this scope will have no effect. Allows for proper scoped objects that are conditionally hidden.
	 */
	FORCEINLINE FSlowTask(float InAmountOfWork, const FText& InDefaultMessage = FText(), bool bInEnabled = true, FFeedbackContext& InContext = *GWarn)
		: DefaultMessage(InDefaultMessage)
		, FrameMessage()
		, TotalAmountOfWork(InAmountOfWork)
		, CompletedWork(0)
		, CurrentFrameScope(0)
		, Visibility(ESlowTaskVisibility::Default)
		, StartTime(FPlatformTime::Seconds())
		, bEnabled(bInEnabled && IsInGameThread())
		, bCreatedDialog(false)		// only set to true if we create a dialog
		, Context(InContext)
	{
		// If we have no work to do ourselves, create an arbitrary scope so that any actions performed underneath this still contribute to this one.
		if (TotalAmountOfWork == 0.f)
		{
			TotalAmountOfWork = CurrentFrameScope = 1.f;
		}
	}

	/** Function that initializes the scope by adding it to its context's stack */
	FORCEINLINE void Initialize()
	{
		if (bEnabled)
		{
			Context.ScopeStack->Push(this);
		}
	}

	/** Function that finishes any remaining work and removes itself from the global scope stack */
	FORCEINLINE void Destroy()
	{
		if (bEnabled)
		{
			if (bCreatedDialog)
			{
				checkSlow(GIsSlowTask);
				Context.FinalizeSlowTask();
			}

			FSlowTaskStack& Stack = *Context.ScopeStack;
			checkSlow(Stack.Num() != 0 && Stack.Last() == this);

			auto* Task = Stack.Last();
			if (ensureMsgf(Task == this, TEXT("Out-of-order scoped task construction/destruction")))
			{
				Stack.Pop(false);
			}
			else
			{
				Stack.RemoveSingleSwap(this, false);
			}

			if (Stack.Num() != 0)
			{
				// Stop anything else contributing to the parent frame
				auto* Parent = Stack.Last();
				Parent->EnterProgressFrame(0, Parent->FrameMessage);
			}

		}
	}

	/**
	 * Creates a new dialog for this slow task, if there is currently not one open
	 * @param		bShowCancelButton		Whether to show a cancel button on the dialog or not
	 * @param		bAllowInPIE				Whether to allow this dialog in PIE. If false, this dialog will not appear during PIE sessions.
	 */
	void MakeDialog(bool bShowCancelButton = false, bool bAllowInPIE = false);

	/**
	 * Indicate that we are to enter a frame that will take up the specified amount of work. Completes any previous frames (potentially contributing to parent scopes' progress).
	 * @param		ExpectedWorkThisFrame	The amount of work that will happen between now and the next frame, as a numerator of TotalAmountOfWork.
	 * @param		Text					Optional text to describe this frame's purpose.
	 */
	FORCEINLINE void EnterProgressFrame(float ExpectedWorkThisFrame = 1.f, FText Text = FText())
	{
		FrameMessage = Text;
		CompletedWork += CurrentFrameScope;

		const float WorkRemaining = TotalAmountOfWork - CompletedWork;
		verifySlow(ExpectedWorkThisFrame <= WorkRemaining);
		CurrentFrameScope = FMath::Min(WorkRemaining, ExpectedWorkThisFrame);

		if (bEnabled)
		{
			Context.RequestUpdateUI(bCreatedDialog || (*Context.ScopeStack)[0] == this);
		}
	}

	/**
	 * Get the frame message or default message if empty
	 */
	FText GetCurrentMessage() const
	{
		return FrameMessage.IsEmpty() ? DefaultMessage : FrameMessage;
	}
};


/**
 * A scope block representing an amount of work divided up into sections.
 * Use one scope at the top of each function to give accurate feedback to the user of a slow operation's progress.
 *
 * Example Usage:
 *	void DoSlowWork()
 *	{
 *		FScopedSlowTask Progress(2.f, LOCTEXT("DoingSlowWork", "Doing Slow Work..."));
 *		// Optionally make this show a dialog if not already shown
 *		Progress.MakeDialog();
 *
 *		// Indicate that we are entering a frame representing 1 unit of work
 *		Progress.EnterProgressFrame(1.f);
 *		
 *		// DoFirstThing() can follow a similar pattern of creating a scope divided into frames. These contribute to their parent's progress frame proportionately.
 *		DoFirstThing();
 *		
 *		Progress.EnterProgressFrame(1.f);
 *		DoSecondThing();
 *	}
 *
 */
struct FScopedSlowTask : FSlowTask
{

	/**
	 * Construct this scope from an amount of work to do, and a message to display
	 * @param		InAmountOfWork			Arbitrary number of work units to perform (can be a percentage or number of steps).
	 *										0 indicates that no progress frames are to be entered in this scope (automatically enters a frame encompassing the entire scope)
	 * @param		InDefaultMessage		A message to display to the user to describe the purpose of the scope
	 * @param		bInEnabled				When false, this scope will have no effect. Allows for proper scoped objects that are conditionally disabled.
	 */
	FORCEINLINE FScopedSlowTask(float InAmountOfWork, const FText& InDefaultMessage = FText(), bool bInEnabled = true, FFeedbackContext& InContext = *GWarn)
		: FSlowTask(InAmountOfWork, InDefaultMessage, bInEnabled, InContext)
	{
		Initialize();
	}

	FORCEINLINE ~FScopedSlowTask()
	{
		Destroy();
	}
};