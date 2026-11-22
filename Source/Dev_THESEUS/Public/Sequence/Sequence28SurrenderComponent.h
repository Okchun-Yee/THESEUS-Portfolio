// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "Sequence28SurrenderComponent.generated.h"

class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSequence28SurrenderWindowStarted, float, TimeLimitSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSequence28SurrenderAccepted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSequence28SurrenderTimeout, int32, WarningShotCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSequence28SurrenderWarningElapsed, float, ElapsedSeconds);

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API USequence28SurrenderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USequence28SurrenderComponent();

	/** Starts ESC surrender window. If PlayerController is null, Player0 is used. */
	UFUNCTION(BlueprintCallable, Category="Sequence28|Surrender")
	bool StartSurrenderWindow(APlayerController* PlayerController = nullptr);

	UFUNCTION(BlueprintCallable, Category="Sequence28|Surrender")
	void CancelSurrenderWindow();

	/** Call this from BP ESC input event. */
	UFUNCTION(BlueprintCallable, Category="Sequence28|Surrender")
	void NotifySurrenderInput();

	UFUNCTION(BlueprintCallable, Category="Sequence28|Surrender")
	bool IsSurrenderWindowActive() const { return bWindowActive; }

	UFUNCTION(BlueprintCallable, Category="Sequence28|Surrender")
	float GetRemainingTime() const { return RemainingTimeSeconds; }

	UPROPERTY(BlueprintAssignable, Category="Sequence28|Surrender")
	FOnSequence28SurrenderWindowStarted OnSurrenderWindowStarted;

	UPROPERTY(BlueprintAssignable, Category="Sequence28|Surrender")
	FOnSequence28SurrenderAccepted OnSurrenderAccepted;

	/** Timeout path: BP handles forced progress/death and 3-shot sky gesture. */
	UPROPERTY(BlueprintAssignable, Category="Sequence28|Surrender")
	FOnSequence28SurrenderTimeout OnSurrenderTimeout;

	/** Fired once when no input has been received for WarningElapsedSeconds. */
	UPROPERTY(BlueprintAssignable, Category="Sequence28|Surrender")
	FOnSequence28SurrenderWarningElapsed OnSurrenderWarningElapsed;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleWindowTick();
	void FinishSurrenderAccepted();
	void FinishSurrenderTimeout();
	void LogRemainingTimeIfNeeded();

private:
	UPROPERTY(EditAnywhere, Category="Sequence28|Surrender", meta=(ClampMin="0.1"))
	float SurrenderTimeLimitSeconds = 10.0f;

	UPROPERTY(EditAnywhere, Category="Sequence28|Surrender", meta=(ClampMin="1"))
	int32 TimeoutWarningShotCount = 3;

	UPROPERTY(EditAnywhere, Category="Sequence28|Surrender", meta=(ClampMin="0.1"))
	float WarningElapsedSeconds = 3.0f;

	UPROPERTY(EditAnywhere, Category="Sequence28|Surrender", meta=(ClampMin="0.01"))
	float TickInterval = 0.05f;

	UPROPERTY(EditAnywhere, Category="Sequence28|Surrender|Debug")
	bool bEnableSurrenderLog = true;

	UPROPERTY(EditAnywhere, Category="Sequence28|Surrender|Debug", meta=(ClampMin="0.1"))
	float RemainingTimeLogIntervalSeconds = 1.0f;

	UPROPERTY(VisibleAnywhere, Category="Sequence28|Surrender")
	bool bWindowActive = false;

	UPROPERTY(VisibleAnywhere, Category="Sequence28|Surrender")
	float RemainingTimeSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, Category="Sequence28|Surrender")
	float ElapsedTimeSeconds = 0.0f;

	float RemainingTimeLogAccumulator = 0.0f;
	bool bWarningElapsedFired = false;

	UPROPERTY()
	TObjectPtr<APlayerController> TargetPlayerController = nullptr;

	FTimerHandle SurrenderTickTimerHandle;
};
