// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieScene3DPathSection.h"
#include "MovieScene3DPathTrack.h"
#include "Components/SplineComponent.h"

UMovieScene3DPathSection::UMovieScene3DPathSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
	, FrontAxisEnum(MovieScene3DPathSection_Axis::Y)
	, UpAxisEnum(MovieScene3DPathSection_Axis::Z)
	, bFollow (true)
	, bReverse (false)
	, bForceUpright (false)
{
}

void UMovieScene3DPathSection::SetPathId(const FGuid& InPathId)
{
	Modify();

	PathId = InPathId;
}

FGuid UMovieScene3DPathSection::GetPathId() const
{
	return PathId;
}

void UMovieScene3DPathSection::Eval( float Position, USplineComponent* SplineComponent, FVector& OutTranslation, FRotator& OutRotation ) const
{
	float Timing = TimingCurve.Eval( Position );

	if (Timing < 0.f)
	{
		Timing = 0.f;
	}
	else if (Timing > 1.f)
	{
		Timing = 1.f;
	}

	if (bReverse)
	{
		Timing = 1.f - Timing;
	}

	const bool UseConstantVelocity = true;
	OutTranslation = SplineComponent->GetWorldLocationAtTime(Timing, UseConstantVelocity);
	OutRotation = SplineComponent->GetWorldRotationAtTime(Timing, UseConstantVelocity);
	
	FMatrix NewRotationMatrix = FRotationMatrix(OutRotation);

	FVector UpAxis(0, 0, 0);
	if (UpAxisEnum == MovieScene3DPathSection_Axis::X)
	{
		UpAxis = FVector(1, 0, 0);
	}
	else if (UpAxisEnum == MovieScene3DPathSection_Axis::NEG_X)
	{
		UpAxis = FVector(-1, 0, 0);
	}
	else if (UpAxisEnum == MovieScene3DPathSection_Axis::Y)
	{
		UpAxis = FVector(0, 1, 0);
	}
	else if (UpAxisEnum == MovieScene3DPathSection_Axis::NEG_Y)
	{
		UpAxis = FVector(0, -1, 0);
	}
	else if (UpAxisEnum == MovieScene3DPathSection_Axis::Z)
	{
		UpAxis = FVector(0, 0, 1);
	}
	else if (UpAxisEnum == MovieScene3DPathSection_Axis::NEG_Z)
	{
		UpAxis = FVector(0, 0, -1);
	}

	FVector FrontAxis(0, 0, 0);
	if (FrontAxisEnum == MovieScene3DPathSection_Axis::X)
	{
		FrontAxis = FVector(1, 0, 0);
	}
	else if (FrontAxisEnum == MovieScene3DPathSection_Axis::NEG_X)
	{
		FrontAxis = FVector(-1, 0, 0);
	}
	else if (FrontAxisEnum == MovieScene3DPathSection_Axis::Y)
	{
		FrontAxis = FVector(0, 1, 0);
	}
	else if (FrontAxisEnum == MovieScene3DPathSection_Axis::NEG_Y)
	{
		FrontAxis = FVector(0, -1, 0);
	}
	else if (FrontAxisEnum == MovieScene3DPathSection_Axis::Z)
	{
		FrontAxis = FVector(0, 0, 1);
	}
	else if (FrontAxisEnum == MovieScene3DPathSection_Axis::NEG_Z)
	{
		FrontAxis = FVector(0, 0, -1);
	}

	// Negate the front axis because the spline rotation comes in reversed
	FrontAxis *= FVector(-1, -1, -1);

	FMatrix AxisRotator = FRotationMatrix::MakeFromXZ(FrontAxis, UpAxis);
	NewRotationMatrix = AxisRotator * NewRotationMatrix;
	OutRotation = NewRotationMatrix.Rotator();

	if (bForceUpright)
	{
		OutRotation.Pitch = 0.f;
		OutRotation.Roll = 0.f;
	}
}

void UMovieScene3DPathSection::MoveSection( float DeltaPosition, TSet<FKeyHandle>& KeyHandles )
{
	Super::MoveSection( DeltaPosition, KeyHandles );

	// Move the curve
	TimingCurve.ShiftCurve(DeltaPosition, KeyHandles);
}

void UMovieScene3DPathSection::DilateSection( float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles )
{	
	Super::DilateSection(DilationFactor, Origin, KeyHandles);
	
	TimingCurve.ScaleCurve(Origin, DilationFactor, KeyHandles);
}

void UMovieScene3DPathSection::GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const
{
	for (auto It(TimingCurve.GetKeyHandleIterator()); It; ++It)
	{
		float Time = TimingCurve.GetKeyTime(It.Key());
		if (IsTimeWithinSection(Time))
		{
			KeyHandles.Add(It.Key());
		}
	}
}

void UMovieScene3DPathSection::AddPath( float Time, float SequenceEndTime, const FGuid& InPathId )
{
	Modify();
	PathId = InPathId;

	TimingCurve.UpdateOrAddKey(Time, 0);
	TimingCurve.UpdateOrAddKey(SequenceEndTime, 1);
}

