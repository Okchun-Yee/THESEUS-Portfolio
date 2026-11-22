// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "Sequence/SequenceStreamingTypes.h"
#include "SequenceLevelStreamingComponent.generated.h"

class AActor;
class USequenceStreamingConfigDataAsset;

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API USequenceLevelStreamingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USequenceLevelStreamingComponent();

	UFUNCTION(BlueprintCallable, Category="Sequence|Streaming")
	void EnterSection(FName SectionId, AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category="Sequence|Streaming")
	void ExitSection(FName SectionId, AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category="Sequence|Streaming")
	void LoadLevels(const TArray<FName>& Levels, bool bBlockOnLoad = false);

	UFUNCTION(BlueprintCallable, Category="Sequence|Streaming")
	void UnloadLevels(const TArray<FName>& Levels, bool bBlockOnUnload = false, float Delay = 0.0f);

	UFUNCTION(BlueprintCallable, Category="Sequence|Streaming")
	void ResetStreamingState();

	UFUNCTION(BlueprintCallable, Category="Sequence|Streaming")
	bool IsLevelTrackedAsLoaded(FName LevelName) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool TryGetSectionRule(FName SectionId, FSequenceSectionRule& OutRule) const;
	void ApplySectionRule(const FSequenceSectionRule& Rule, ESequenceSectionTriggerPhase Phase);

	void RequestLoadLevel(FName LevelName, bool bBlockOnLoad);
	void RequestUnloadLevel(FName LevelName, bool bBlockOnUnload, float Delay);

	void StreamLoadLevel(FName LevelName, bool bBlockOnLoad);
	void StreamUnloadLevel(FName LevelName, bool bBlockOnUnload);

	void CancelPendingUnload(FName LevelName);
	void HandleDelayedUnload(FName LevelName);

	void GatherUniqueLevels(const TArray<FName>& InLevels, TArray<FName>& OutLevels) const;
	bool IsInstigatorAllowedByRule(const FSequenceSectionRule& Rule, AActor* InstigatorActor) const;

private:
	UPROPERTY(EditAnywhere, Category="Sequence|Streaming")
	TObjectPtr<USequenceStreamingConfigDataAsset> StreamingConfig = nullptr;

	UPROPERTY(EditAnywhere, Category="Sequence|Streaming")
	bool bLoadInitialLevelsOnBeginPlay = false;

	UPROPERTY(EditAnywhere, Category="Sequence|Streaming|Debug")
	bool bEnableStreamingLog = true;

	UPROPERTY(VisibleAnywhere, Category="Sequence|Streaming")
	TMap<FName, int32> LevelRefCountMap;

	UPROPERTY(VisibleAnywhere, Category="Sequence|Streaming")
	TSet<FName> ActiveSectionIds;

	TMap<FName, FTimerHandle> PendingUnloadTimerMap;
	TMap<FName, bool> PendingUnloadBlockMap;

	int32 NextLatentUUID = 1;
};
