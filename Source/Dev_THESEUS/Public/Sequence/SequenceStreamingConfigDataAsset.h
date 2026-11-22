// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sequence/SequenceStreamingTypes.h"
#include "SequenceStreamingConfigDataAsset.generated.h"

UCLASS(BlueprintType)
class DEV_THESEUS_API USequenceStreamingConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sequence|Streaming")
	TMap<FName, FSequenceSectionRule> SectionRules;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sequence|Streaming")
	TArray<FName> InitialLoadLevels;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sequence|Streaming")
	bool bInitialLoadBlockOnLoad = false;
};
