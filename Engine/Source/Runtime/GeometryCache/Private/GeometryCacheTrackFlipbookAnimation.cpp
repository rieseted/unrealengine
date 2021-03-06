// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GeometryCacheModulePrivatePCH.h"
#include "GeometryCacheMeshData.h"

#include "GeometryCacheTrackFlipbookAnimation.h"

GEOMETRYCACHE_API UGeometryCacheTrack_FlipbookAnimation::UGeometryCacheTrack_FlipbookAnimation(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/) : UGeometryCacheTrack(ObjectInitializer)
{
	NumMeshSamples = 0;
}

UGeometryCacheTrack_FlipbookAnimation::~UGeometryCacheTrack_FlipbookAnimation()
{
	NumMeshSamples = 0;
	MeshSamples.Empty();
	MeshSampleTimes.Empty();
}

SIZE_T UGeometryCacheTrack_FlipbookAnimation::GetResourceSize(EResourceSizeMode::Type Mode)
{
	// Determine resource size according to what is actually serialized
	SIZE_T ResourceSize = 0;
	ResourceSize += UGeometryCacheTrack::GetResourceSize(Mode);
	for (int32 SampleIndex = 0; SampleIndex < MeshSamples.Num(); ++SampleIndex )
	{
		ResourceSize += MeshSamples[SampleIndex].GetResourceSize();
	}
	ResourceSize += sizeof(MeshSamples);
	ResourceSize += MeshSampleTimes.Num() * sizeof(float);
	ResourceSize += sizeof(MeshSampleTimes);
	ResourceSize += sizeof(NumMeshSamples);

	return ResourceSize;
}

void UGeometryCacheTrack_FlipbookAnimation::Serialize(FArchive& Ar)
{
	UGeometryCacheTrack::Serialize(Ar);
	
	Ar << NumMeshSamples;
	Ar << MeshSamples;
	Ar << MeshSampleTimes;
}

const bool UGeometryCacheTrack_FlipbookAnimation::UpdateMeshData(const float Time, const bool bLooping, int32& InOutMeshSampleIndex, FGeometryCacheMeshData*& OutMeshData)
{
	// Retrieve sample index from Time
	const int32 MeshSampleIndex = FindSampleIndexFromTime(MeshSampleTimes, Time, bLooping);

	// Update the Vertices and Index if MeshSampleIndex is different from the stored InOutMeshSampleIndex
	if (MeshSampleIndex != InOutMeshSampleIndex)
	{
		OutMeshData = &MeshSamples[MeshSampleIndex];
		InOutMeshSampleIndex = MeshSampleIndex;

		return true;
	}

	return false;
}

const float UGeometryCacheTrack_FlipbookAnimation::GetMaxSampleTime() const
{
	const float BaseTime = UGeometryCacheTrack::GetMaxSampleTime();

	if (MeshSampleTimes.Num() > 0)
	{
		const float MeshSampleTime = MeshSampleTimes.Last();
		return (BaseTime > MeshSampleTime) ? BaseTime : MeshSampleTime;
	}

	return BaseTime;
}

void UGeometryCacheTrack_FlipbookAnimation::AddMeshSample(const FGeometryCacheMeshData& MeshData, const float SampleTime)
{
	MeshSamples.Add(MeshData);
	MeshSampleTimes.Add(SampleTime);
	NumMeshSamples++;

	// Store the total number of materials within this track
	if (MeshData.BatchesInfo.Num() > (int32)NumMaterials)
	{
		NumMaterials = MeshData.BatchesInfo.Num();
	}
}

void UGeometryCacheTrack_FlipbookAnimation::BeginDestroy()
{
	Super::BeginDestroy();
	NumMeshSamples = 0;
	MeshSamples.Empty();
	MeshSampleTimes.Empty();
}
