// Copyright 2026, Okchun-Yee.

#include "Sequence/SequenceCutsceneChainComponent.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

USequenceCutsceneChainComponent::USequenceCutsceneChainComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool USequenceCutsceneChainComponent::StartChain(APlayerController* PlayerController)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	CancelChain();

	TargetPlayerController = PlayerController ? PlayerController : World->GetFirstPlayerController();
	if (!IsValid(TargetPlayerController))
	{
		if (bEnableChainLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CutsceneChain] Start failed: PlayerController invalid."));
		}
		return false;
	}

	bChainActive = true;
	bChainCompleted = false;
	StepLinkedStates.Empty();
	StepLinkedStates.Init(false, FMath::Max(0, StepNames.Num()));

	ApplyInputLock(bLockInputDuringChain);

	if (bEnableChainLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[CutsceneChain] Chain started. StepCount=%d"), StepNames.Num());
	}

	return true;
}

void USequenceCutsceneChainComponent::CancelChain()
{
	ApplyInputLock(false);

	bChainActive = false;
	bChainCompleted = false;
	StepLinkedStates.Empty();
	TargetPlayerController = nullptr;
}

bool USequenceCutsceneChainComponent::MarkStepLinkedByIndex(const int32 StepIndex)
{
	return MarkStepInternal(StepIndex);
}

bool USequenceCutsceneChainComponent::MarkStepLinkedByName(const FName StepName)
{
	const int32 StepIndex = StepNames.IndexOfByKey(StepName);
	if (StepIndex == INDEX_NONE)
	{
		if (bEnableChainLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CutsceneChain] Step name not found: %s"), *StepName.ToString());
		}
		return false;
	}

	return MarkStepInternal(StepIndex);
}

bool USequenceCutsceneChainComponent::IsStepLinked(const int32 StepIndex) const
{
	return StepLinkedStates.IsValidIndex(StepIndex) ? StepLinkedStates[StepIndex] : false;
}

FName USequenceCutsceneChainComponent::GetStepName(const int32 StepIndex) const
{
	return StepNames.IsValidIndex(StepIndex) ? StepNames[StepIndex] : NAME_None;
}

void USequenceCutsceneChainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelChain();
	Super::EndPlay(EndPlayReason);
}

bool USequenceCutsceneChainComponent::MarkStepInternal(const int32 StepIndex)
{
	if (!bChainActive || bChainCompleted)
	{
		if (bEnableChainLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CutsceneChain] Step ignored: chain inactive or completed. Step=%d"), StepIndex);
		}
		return false;
	}

	if (!StepNames.IsValidIndex(StepIndex) || !StepLinkedStates.IsValidIndex(StepIndex))
	{
		if (bEnableChainLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CutsceneChain] Step index invalid: %d"), StepIndex);
		}
		return false;
	}

	if (!ValidateStepOrder(StepIndex))
	{
		if (bEnableChainLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CutsceneChain] Step order invalid. Step=%d"), StepIndex);
		}
		return false;
	}

	if (StepLinkedStates[StepIndex])
	{
		return true;
	}

	StepLinkedStates[StepIndex] = true;
	OnStepLinked.Broadcast(StepIndex, StepNames[StepIndex]);

	if (bEnableChainLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[CutsceneChain] Step linked: Index=%d Name=%s"),
			StepIndex, *StepNames[StepIndex].ToString());
	}

	bool bAllLinked = true;
	for (const bool bLinked : StepLinkedStates)
	{
		if (!bLinked)
		{
			bAllLinked = false;
			break;
		}
	}

	if (bAllLinked)
	{
		CompleteChain();
	}

	return true;
}

bool USequenceCutsceneChainComponent::ValidateStepOrder(const int32 StepIndex) const
{
	if (!bRequireOrderedFlow)
	{
		return true;
	}

	for (int32 PrevIndex = 0; PrevIndex < StepIndex; ++PrevIndex)
	{
		if (!StepLinkedStates.IsValidIndex(PrevIndex) || !StepLinkedStates[PrevIndex])
		{
			return false;
		}
	}

	return true;
}

void USequenceCutsceneChainComponent::ApplyInputLock(const bool bLock)
{
	if (!IsValid(TargetPlayerController))
	{
		return;
	}

	if (bLock)
	{
		TargetPlayerController->SetIgnoreMoveInput(true);
		TargetPlayerController->SetIgnoreLookInput(true);
	}
	else
	{
		TargetPlayerController->SetIgnoreMoveInput(false);
		TargetPlayerController->SetIgnoreLookInput(false);
	}
}

void USequenceCutsceneChainComponent::CompleteChain()
{
	if (bChainCompleted)
	{
		return;
	}

	bChainCompleted = true;
	bChainActive = false;
	ApplyInputLock(false);

	if (bEnableChainLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[CutsceneChain] Chain completed."));
	}

	OnChainCompleted.Broadcast();
}
