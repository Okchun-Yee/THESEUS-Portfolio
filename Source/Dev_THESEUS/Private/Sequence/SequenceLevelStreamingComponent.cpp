// Copyright 2026, Okchun-Yee.

#include "Sequence/SequenceLevelStreamingComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sequence/SequenceStreamingConfigDataAsset.h"
#include "TimerManager.h"

USequenceLevelStreamingComponent::USequenceLevelStreamingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USequenceLevelStreamingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bLoadInitialLevelsOnBeginPlay && IsValid(StreamingConfig))
	{
		LoadLevels(StreamingConfig->InitialLoadLevels, StreamingConfig->bInitialLoadBlockOnLoad);
	}
}

void USequenceLevelStreamingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetStreamingState();
	Super::EndPlay(EndPlayReason);
}

void USequenceLevelStreamingComponent::EnterSection(FName SectionId, AActor* InstigatorActor)
{
	if (ActiveSectionIds.Contains(SectionId))
	{
		if (bEnableStreamingLog)
		{
			UE_LOG(LogTemp, Log, TEXT("[SequenceStream] EnterSection ignored (already active): SectionId=%s"), *SectionId.ToString());
		}
		return;
	}

	FSequenceSectionRule Rule;
	if (!TryGetSectionRule(SectionId, Rule))
	{
		if (bEnableStreamingLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SequenceStream] EnterSection skipped: missing rule for SectionId=%s"), *SectionId.ToString());
		}
		return;
	}

	if (!IsInstigatorAllowedByRule(Rule, InstigatorActor))
	{
		return;
	}

	ApplySectionRule(Rule, ESequenceSectionTriggerPhase::Enter);
	ActiveSectionIds.Add(SectionId);
}

void USequenceLevelStreamingComponent::ExitSection(FName SectionId, AActor* InstigatorActor)
{
	if (!ActiveSectionIds.Contains(SectionId))
	{
		if (bEnableStreamingLog)
		{
			UE_LOG(LogTemp, Log, TEXT("[SequenceStream] ExitSection ignored (not active): SectionId=%s"), *SectionId.ToString());
		}
		return;
	}

	FSequenceSectionRule Rule;
	if (!TryGetSectionRule(SectionId, Rule))
	{
		if (bEnableStreamingLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SequenceStream] ExitSection skipped: missing rule for SectionId=%s"), *SectionId.ToString());
		}
		return;
	}

	if (!IsInstigatorAllowedByRule(Rule, InstigatorActor))
	{
		return;
	}

	ActiveSectionIds.Remove(SectionId);
	ApplySectionRule(Rule, ESequenceSectionTriggerPhase::Exit);
}

void USequenceLevelStreamingComponent::LoadLevels(const TArray<FName>& Levels, bool bBlockOnLoad)
{
	TArray<FName> UniqueLevels;
	GatherUniqueLevels(Levels, UniqueLevels);

	for (const FName LevelName : UniqueLevels)
	{
		RequestLoadLevel(LevelName, bBlockOnLoad);
	}
}

void USequenceLevelStreamingComponent::UnloadLevels(const TArray<FName>& Levels, bool bBlockOnUnload, float Delay)
{
	TArray<FName> UniqueLevels;
	GatherUniqueLevels(Levels, UniqueLevels);

	for (const FName LevelName : UniqueLevels)
	{
		RequestUnloadLevel(LevelName, bBlockOnUnload, Delay);
	}
}

void USequenceLevelStreamingComponent::ResetStreamingState()
{
	if (UWorld* World = GetWorld())
	{
		for (TPair<FName, FTimerHandle>& PendingPair : PendingUnloadTimerMap)
		{
			World->GetTimerManager().ClearTimer(PendingPair.Value);
		}
	}

	PendingUnloadTimerMap.Reset();
	PendingUnloadBlockMap.Reset();
	LevelRefCountMap.Reset();
	ActiveSectionIds.Reset();
}

bool USequenceLevelStreamingComponent::IsLevelTrackedAsLoaded(FName LevelName) const
{
	const int32* RefCount = LevelRefCountMap.Find(LevelName);
	return RefCount != nullptr && *RefCount > 0;
}

bool USequenceLevelStreamingComponent::TryGetSectionRule(FName SectionId, FSequenceSectionRule& OutRule) const
{
	if (!IsValid(StreamingConfig))
	{
		return false;
	}

	if (const FSequenceSectionRule* FoundRule = StreamingConfig->SectionRules.Find(SectionId))
	{
		OutRule = *FoundRule;
		return true;
	}

	return false;
}

void USequenceLevelStreamingComponent::ApplySectionRule(const FSequenceSectionRule& Rule, ESequenceSectionTriggerPhase Phase)
{
	TSet<FName> RequestedLoadLevels;
	TSet<FName> RequestedUnloadLevels;

	auto RequestUniqueLoadList = [this, &RequestedLoadLevels](const TArray<FName>& InLevels, bool bBlockOnLoad, const TCHAR* SourceLabel)
	{
		TArray<FName> UniqueLevels;
		GatherUniqueLevels(InLevels, UniqueLevels);

		for (const FName LevelName : UniqueLevels)
		{
			if (RequestedLoadLevels.Contains(LevelName))
			{
				if (bEnableStreamingLog)
				{
					UE_LOG(LogTemp, Warning, TEXT("[SequenceStream] Duplicate load request in same phase ignored: Level=%s Source=%s"), *LevelName.ToString(), SourceLabel);
				}
				continue;
			}

			RequestedLoadLevels.Add(LevelName);
			RequestLoadLevel(LevelName, bBlockOnLoad);
		}
	};

	auto RequestUniqueUnloadList = [this, &RequestedUnloadLevels](const TArray<FName>& InLevels, bool bBlockOnUnload, float Delay, const TCHAR* SourceLabel)
	{
		TArray<FName> UniqueLevels;
		GatherUniqueLevels(InLevels, UniqueLevels);

		for (const FName LevelName : UniqueLevels)
		{
			if (RequestedUnloadLevels.Contains(LevelName))
			{
				if (bEnableStreamingLog)
				{
					UE_LOG(LogTemp, Warning, TEXT("[SequenceStream] Duplicate unload request in same phase ignored: Level=%s Source=%s"), *LevelName.ToString(), SourceLabel);
				}
				continue;
			}

			RequestedUnloadLevels.Add(LevelName);
			RequestUnloadLevel(LevelName, bBlockOnUnload, Delay);
		}
	};

	if (Phase == ESequenceSectionTriggerPhase::Enter)
	{
		RequestUniqueLoadList(Rule.EnterLoadLevels, Rule.bBlockOnLoad, TEXT("EnterLoadLevels"));
		RequestUniqueLoadList(Rule.PreloadLevels, false, TEXT("PreloadLevels"));
		RequestUniqueLoadList(Rule.ExtraLoadLevels, Rule.bBlockOnLoad, TEXT("ExtraLoadLevels"));
		RequestUniqueUnloadList(Rule.EnterUnloadLevels, Rule.bBlockOnUnload, Rule.UnloadDelay, TEXT("EnterUnloadLevels"));
	}
	else
	{
		RequestUniqueLoadList(Rule.ExitLoadLevels, Rule.bBlockOnLoad, TEXT("ExitLoadLevels"));
		RequestUniqueUnloadList(Rule.ExitUnloadLevels, Rule.bBlockOnUnload, Rule.UnloadDelay, TEXT("ExitUnloadLevels"));
		RequestUniqueUnloadList(Rule.ExtraUnloadLevels, Rule.bBlockOnUnload, Rule.UnloadDelay, TEXT("ExtraUnloadLevels"));
	}
}

void USequenceLevelStreamingComponent::RequestLoadLevel(FName LevelName, bool bBlockOnLoad)
{
	if (LevelName.IsNone())
	{
		return;
	}

	CancelPendingUnload(LevelName);

	int32& RefCount = LevelRefCountMap.FindOrAdd(LevelName);
	const bool bWasUntracked = RefCount <= 0;
	++RefCount;

	if (bWasUntracked)
	{
		StreamLoadLevel(LevelName, bBlockOnLoad);
	}

	if (bEnableStreamingLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[SequenceStream] LoadRef++ Level=%s Ref=%d"), *LevelName.ToString(), RefCount);
	}
}

void USequenceLevelStreamingComponent::RequestUnloadLevel(FName LevelName, bool bBlockOnUnload, float Delay)
{
	if (LevelName.IsNone())
	{
		return;
	}

	int32* RefCountPtr = LevelRefCountMap.Find(LevelName);
	if (!RefCountPtr)
	{
		return;
	}

	int32& RefCount = *RefCountPtr;
	RefCount = FMath::Max(RefCount - 1, 0);

	if (bEnableStreamingLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[SequenceStream] LoadRef-- Level=%s Ref=%d"), *LevelName.ToString(), RefCount);
	}

	if (RefCount > 0)
	{
		return;
	}

	if (Delay <= KINDA_SMALL_NUMBER)
	{
		StreamUnloadLevel(LevelName, bBlockOnUnload);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	CancelPendingUnload(LevelName);
	PendingUnloadBlockMap.Add(LevelName, bBlockOnUnload);

	FTimerDelegate DelayDelegate;
	DelayDelegate.BindUObject(this, &USequenceLevelStreamingComponent::HandleDelayedUnload, LevelName);

	FTimerHandle DelayHandle;
	World->GetTimerManager().SetTimer(DelayHandle, DelayDelegate, Delay, false);
	PendingUnloadTimerMap.Add(LevelName, DelayHandle);
}

void USequenceLevelStreamingComponent::StreamLoadLevel(FName LevelName, bool bBlockOnLoad)
{
	UObject* WorldContext = GetOwner() ? static_cast<UObject*>(GetOwner()) : static_cast<UObject*>(this);

	FLatentActionInfo LatentInfo;
	LatentInfo.UUID = NextLatentUUID++;

	UGameplayStatics::LoadStreamLevel(WorldContext, LevelName, true, bBlockOnLoad, LatentInfo);
}

void USequenceLevelStreamingComponent::StreamUnloadLevel(FName LevelName, bool bBlockOnUnload)
{
	UObject* WorldContext = GetOwner() ? static_cast<UObject*>(GetOwner()) : static_cast<UObject*>(this);

	FLatentActionInfo LatentInfo;
	LatentInfo.UUID = NextLatentUUID++;

	UGameplayStatics::UnloadStreamLevel(WorldContext, LevelName, LatentInfo, bBlockOnUnload);
}

void USequenceLevelStreamingComponent::CancelPendingUnload(FName LevelName)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		PendingUnloadTimerMap.Remove(LevelName);
		PendingUnloadBlockMap.Remove(LevelName);
		return;
	}

	if (FTimerHandle* PendingTimer = PendingUnloadTimerMap.Find(LevelName))
	{
		World->GetTimerManager().ClearTimer(*PendingTimer);
		PendingUnloadTimerMap.Remove(LevelName);
	}

	PendingUnloadBlockMap.Remove(LevelName);
}

void USequenceLevelStreamingComponent::HandleDelayedUnload(FName LevelName)
{
	PendingUnloadTimerMap.Remove(LevelName);

	const int32* RefCount = LevelRefCountMap.Find(LevelName);
	if (!RefCount || *RefCount > 0)
	{
		PendingUnloadBlockMap.Remove(LevelName);
		return;
	}

	const bool bBlockOnUnload = PendingUnloadBlockMap.FindRef(LevelName);
	PendingUnloadBlockMap.Remove(LevelName);

	StreamUnloadLevel(LevelName, bBlockOnUnload);
}

void USequenceLevelStreamingComponent::GatherUniqueLevels(const TArray<FName>& InLevels, TArray<FName>& OutLevels) const
{
	OutLevels.Reset();

	TSet<FName> UniqueNames;
	for (const FName LevelName : InLevels)
	{
		if (!LevelName.IsNone())
		{
			UniqueNames.Add(LevelName);
		}
	}

	OutLevels.Reserve(UniqueNames.Num());
	for (const FName& UniqueLevelName : UniqueNames)
	{
		OutLevels.Add(UniqueLevelName);
	}
}

bool USequenceLevelStreamingComponent::IsInstigatorAllowedByRule(const FSequenceSectionRule& Rule, AActor* InstigatorActor) const
{
	if (!Rule.bPlayerOnly)
	{
		return true;
	}

	const APawn* InstigatorPawn = Cast<APawn>(InstigatorActor);
	return IsValid(InstigatorPawn) && InstigatorPawn->IsPlayerControlled();
}
