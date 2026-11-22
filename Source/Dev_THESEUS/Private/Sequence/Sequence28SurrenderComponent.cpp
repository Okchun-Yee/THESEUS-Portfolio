// Copyright 2026, Okchun-Yee.

#include "Sequence/Sequence28SurrenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

USequence28SurrenderComponent::USequence28SurrenderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool USequence28SurrenderComponent::StartSurrenderWindow(APlayerController* PlayerController)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	CancelSurrenderWindow();

	TargetPlayerController = PlayerController ? PlayerController : World->GetFirstPlayerController();
	if (!IsValid(TargetPlayerController))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sequence28Surrender] Start failed: PlayerController invalid."));
		return false;
	}

	RemainingTimeSeconds = FMath::Max(0.1f, SurrenderTimeLimitSeconds);
	ElapsedTimeSeconds = 0.0f;
	RemainingTimeLogAccumulator = 0.0f;
	bWarningElapsedFired = false;
	bWindowActive = true;

	World->GetTimerManager().SetTimer(
		SurrenderTickTimerHandle,
		this,
		&USequence28SurrenderComponent::HandleWindowTick,
		FMath::Max(0.01f, TickInterval),
		true);

	OnSurrenderWindowStarted.Broadcast(RemainingTimeSeconds);

	if (bEnableSurrenderLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[Sequence28Surrender] Window started: TimeLimit=%.2f"), RemainingTimeSeconds);
	}

	return true;
}

void USequence28SurrenderComponent::CancelSurrenderWindow()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SurrenderTickTimerHandle);
	}

	bWindowActive = false;
	RemainingTimeSeconds = 0.0f;
	ElapsedTimeSeconds = 0.0f;
	RemainingTimeLogAccumulator = 0.0f;
	bWarningElapsedFired = false;
	TargetPlayerController = nullptr;
}

void USequence28SurrenderComponent::NotifySurrenderInput()
{
	if (!bWindowActive)
	{
		return;
	}

	FinishSurrenderAccepted();
}

void USequence28SurrenderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelSurrenderWindow();
	Super::EndPlay(EndPlayReason);
}

void USequence28SurrenderComponent::HandleWindowTick()
{
	if (!bWindowActive)
	{
		return;
	}

	const float SafeTickInterval = FMath::Max(0.01f, TickInterval);
	ElapsedTimeSeconds += SafeTickInterval;
	RemainingTimeSeconds -= FMath::Max(0.01f, TickInterval);

	if (!bWarningElapsedFired && ElapsedTimeSeconds >= FMath::Max(0.1f, WarningElapsedSeconds))
	{
		bWarningElapsedFired = true;
		OnSurrenderWarningElapsed.Broadcast(ElapsedTimeSeconds);

		if (bEnableSurrenderLog)
		{
			UE_LOG(LogTemp, Log, TEXT("[Sequence28Surrender] Warning elapsed fired at %.2fs"), ElapsedTimeSeconds);
		}
	}

	LogRemainingTimeIfNeeded();
	if (RemainingTimeSeconds <= 0.0f)
	{
		FinishSurrenderTimeout();
	}
}

void USequence28SurrenderComponent::LogRemainingTimeIfNeeded()
{
	if (!bEnableSurrenderLog || !bWindowActive)
	{
		return;
	}

	const float SafeLogInterval = FMath::Max(0.1f, RemainingTimeLogIntervalSeconds);
	RemainingTimeLogAccumulator += FMath::Max(0.01f, TickInterval);
	if (RemainingTimeLogAccumulator < SafeLogInterval)
	{
		return;
	}

	RemainingTimeLogAccumulator = 0.0f;
	UE_LOG(LogTemp, Log, TEXT("[Sequence28Surrender] Waiting input... Remaining=%.1fs"), FMath::Max(0.0f, RemainingTimeSeconds));
}

void USequence28SurrenderComponent::FinishSurrenderAccepted()
{
	if (!bWindowActive)
	{
		return;
	}

	if (bEnableSurrenderLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[Sequence28Surrender] Surrender accepted via input."));
	}

	CancelSurrenderWindow();
	OnSurrenderAccepted.Broadcast();
}

void USequence28SurrenderComponent::FinishSurrenderTimeout()
{
	if (!bWindowActive)
	{
		return;
	}

	if (bEnableSurrenderLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sequence28Surrender] Timeout. WarningShotCount=%d"), TimeoutWarningShotCount);
	}

	CancelSurrenderWindow();
	OnSurrenderTimeout.Broadcast(FMath::Max(1, TimeoutWarningShotCount));
}
