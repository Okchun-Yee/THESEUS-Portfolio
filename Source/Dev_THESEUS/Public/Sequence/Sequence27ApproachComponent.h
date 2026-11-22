// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "UObject/WeakObjectPtr.h"
#include "Sequence27ApproachComponent.generated.h"

class AActor;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSequence27ApproachCompleted, int32, InRangeUnits, int32, ValidUnits);

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API USequence27ApproachComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USequence27ApproachComponent();

	/**
	 * Starts approach phase for provided spawned units.
	 * Each unit moves toward TargetActor until encircle condition is satisfied.
	 */
	UFUNCTION(BlueprintCallable, Category="Sequence27|Approach")
	bool StartApproach(const TArray<APawn*>& InSpawnedUnits, AActor* TargetActor);

	/** Applies eyes-on/off state to provided units if they implement SetEyesActive(bool). */
	UFUNCTION(BlueprintCallable, Category="Sequence27|Approach|Observe")
	void SetEyesActiveForUnits(const TArray<APawn*>& InUnits, bool bActive);

	/** Applies observe-mode stop/brain-stop/look-at flow to provided units. */
	UFUNCTION(BlueprintCallable, Category="Sequence27|Approach|Observe")
	void EnterObserveModeForUnits(const TArray<APawn*>& InUnits, AActor* TargetActor, bool bEnableEyes);

	/** Convenience wrapper for owners that also have AISpawnComponent. */
	UFUNCTION(BlueprintCallable, Category="Sequence27|Approach|Observe")
	void SetEyesActiveForSpawnedUnits(bool bActive);

	/** Convenience wrapper for owners that also have AISpawnComponent. */
	UFUNCTION(BlueprintCallable, Category="Sequence27|Approach|Observe")
	void EnterObserveModeForSpawnedUnits(AActor* TargetActor, bool bEnableEyes);

	/** Stops active approach phase and clears timer. */
	UFUNCTION(BlueprintCallable, Category="Sequence27|Approach")
	void StopApproach();

	UFUNCTION(BlueprintCallable, Category="Sequence27|Approach")
	bool IsApproachActive() const { return bApproachActive; }

	UFUNCTION(BlueprintCallable, Category="Sequence27|Approach")
	int32 GetInRangeUnitCount() const { return LastInRangeUnits; }

	UFUNCTION(BlueprintCallable, Category="Sequence27|Approach")
	int32 GetValidUnitCount() const { return LastValidUnits; }

	/** Broadcasted once when approach condition is met. */
	UPROPERTY(BlueprintAssignable, Category="Sequence27|Approach")
	FOnSequence27ApproachCompleted OnApproachCompleted;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void UpdateApproach();
	void StopApproachInternal(bool bReachedCondition);
	bool ShouldCompleteApproach(int32 InRangeUnits, int32 ValidUnits) const;
	void StopUnitMovement(APawn* Unit, bool bDisableMovement) const;
	void EnterObserveModeForUnit(APawn* Unit) const;
	void TryDirectMoveToTarget(APawn* Unit, const FVector& TargetLocation, float DeltaTime) const;
	void ApplyLookAtTarget(APawn* Unit, AActor* Target) const;
	bool TrySetEyesActive(APawn* Unit, bool bActive) const;

private:
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach", meta=(ClampMin="0.05"))
	float UpdateInterval = 0.2f;

	/** MoveTo acceptance radius used during approach phase. */
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach", meta=(ClampMin="10.0"))
	float ApproachAcceptanceRadius = 220.0f;

	/** Distance threshold that triggers observe/encircle transition. */
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach", meta=(ClampMin="10.0"))
	float EncircleTriggerDistance = 420.0f;

	/** true: all valid units must be within EncircleTriggerDistance. */
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach")
	bool bRequireAllUnitsInRange = true;

	/** Used only when bRequireAllUnitsInRange is false. */
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach", meta=(ClampMin="1"))
	int32 MinUnitsInRange = 1;

	/** Safety timeout. 0 disables timeout-based completion. */
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach", meta=(ClampMin="0.0"))
	float MaxApproachDuration = 12.0f;

	/** Stop BT logic once units reach observe distance so normal chase cannot resume. */
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach|Observe")
	bool bStopBrainLogicOnCompletion = true;

	/** Disable character movement after observe distance is reached. Matches the old BP EnterObserveMode behavior. */
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach|Observe")
	bool bDisableMovementOnCompletion = true;

	UPROPERTY(EditAnywhere, Category="Sequence27|Approach|Observe")
	FString BrainStopReason = TEXT("Sequence27_Observe");

	/** If MoveTo fails (e.g., nav path unavailable), use direct movement fallback. */
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach")
	bool bUseDirectMoveFallbackOnMoveFail = true;

	/** Avoid re-issuing MoveTo every tick while controller is already moving. */
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach")
	bool bOnlyRequestMoveWhenNeeded = true;

	UPROPERTY(EditAnywhere, Category="Sequence27|Approach", meta=(ClampMin="10.0"))
	float DirectMoveSpeed = 520.0f;

	/** Prefer movement-input based fallback for natural locomotion animation. */
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach")
	bool bUseInputDrivenFallback = true;

	/** If distance does not decrease enough for this many ticks, treat unit as stuck. */
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach", meta=(ClampMin="1"))
	int32 StuckTickThreshold = 3;

	/** Minimum distance delta per tick to consider unit progressing. */
	UPROPERTY(EditAnywhere, Category="Sequence27|Approach", meta=(ClampMin="0.0"))
	float MinProgressDistancePerTick = 8.0f;

	UPROPERTY(EditAnywhere, Category="Sequence27|Approach|Debug")
	bool bEnableApproachLog = true;

	UPROPERTY()
	TArray<TObjectPtr<APawn>> ManagedUnits;

	UPROPERTY()
	TObjectPtr<AActor> ObserveTarget = nullptr;

	UPROPERTY(VisibleAnywhere, Category="Sequence27|Approach")
	bool bApproachActive = false;

	UPROPERTY(VisibleAnywhere, Category="Sequence27|Approach")
	int32 LastInRangeUnits = 0;

	UPROPERTY(VisibleAnywhere, Category="Sequence27|Approach")
	int32 LastValidUnits = 0;

	UPROPERTY(VisibleAnywhere, Category="Sequence27|Approach")
	float ElapsedApproachTime = 0.0f;

	TMap<TWeakObjectPtr<APawn>, float> LastDistanceByUnit;
	TMap<TWeakObjectPtr<APawn>, int32> StuckTicksByUnit;

	FTimerHandle ApproachTimerHandle;
};
