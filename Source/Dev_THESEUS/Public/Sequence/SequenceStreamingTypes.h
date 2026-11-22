// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "SequenceStreamingTypes.generated.h"

UENUM(BlueprintType)
enum class ESequenceSectionTriggerPhase : uint8
{
	Enter,
	Exit
};

USTRUCT(BlueprintType)
struct DEV_THESEUS_API FSequenceSectionRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming")
	FName SectionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming")
	TArray<FName> EnterLoadLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming")
	TArray<FName> EnterUnloadLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming")
	TArray<FName> ExitLoadLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming")
	TArray<FName> ExitUnloadLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming")
	TArray<FName> PreloadLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming")
	TArray<FName> ExtraLoadLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming")
	TArray<FName> ExtraUnloadLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming")
	bool bPlayerOnly = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming")
	bool bBlockOnLoad = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming")
	bool bBlockOnUnload = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming", meta=(ClampMin="0.0"))
	float UnloadDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Streaming")
	int32 Priority = 0;
};
