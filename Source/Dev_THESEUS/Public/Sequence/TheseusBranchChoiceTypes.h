// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "TheseusBranchChoiceTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class ETheseusBranchSide : uint8
{
	None,
	Left,
	Right
};

USTRUCT(BlueprintType)
struct DEV_THESEUS_API FTheseusBranchSideRuntimeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Branch")
	ETheseusBranchSide BranchSide = ETheseusBranchSide::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Branch")
	TObjectPtr<AActor> TransparentWallActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Branch")
	TObjectPtr<AActor> EncounterSpawnActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Branch")
	TObjectPtr<AActor> Sequence27Actor = nullptr;
};
