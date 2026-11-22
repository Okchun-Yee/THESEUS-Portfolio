// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SequenceCutsceneChainComponent.generated.h"

class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCutsceneChainStepLinked, int32, StepIndex, FName, StepName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCutsceneChainCompleted);

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API USequenceCutsceneChainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USequenceCutsceneChainComponent();

	/** Starts cutscene chain and resets previous state. */
	UFUNCTION(BlueprintCallable, Category="Sequence|CutsceneChain")
	bool StartChain(APlayerController* PlayerController = nullptr);

	/** Cancels active chain and releases optional input lock. */
	UFUNCTION(BlueprintCallable, Category="Sequence|CutsceneChain")
	void CancelChain();

	/** Marks a step by index. Returns false when order/state validation fails. */
	UFUNCTION(BlueprintCallable, Category="Sequence|CutsceneChain")
	bool MarkStepLinkedByIndex(int32 StepIndex);

	/** Marks a step by step name in StepNames. Returns false when not found/invalid. */
	UFUNCTION(BlueprintCallable, Category="Sequence|CutsceneChain")
	bool MarkStepLinkedByName(FName StepName);

	UFUNCTION(BlueprintCallable, Category="Sequence|CutsceneChain")
	int32 GetStepCount() const { return StepNames.Num(); }

	UFUNCTION(BlueprintCallable, Category="Sequence|CutsceneChain")
	bool IsChainActive() const { return bChainActive; }

	UFUNCTION(BlueprintCallable, Category="Sequence|CutsceneChain")
	bool IsChainCompleted() const { return bChainCompleted; }

	UFUNCTION(BlueprintCallable, Category="Sequence|CutsceneChain")
	bool IsStepLinked(int32 StepIndex) const;

	UFUNCTION(BlueprintCallable, Category="Sequence|CutsceneChain")
	FName GetStepName(int32 StepIndex) const;

	UPROPERTY(BlueprintAssignable, Category="Sequence|CutsceneChain")
	FOnCutsceneChainStepLinked OnStepLinked;

	/** Fired once after all required steps are linked. */
	UPROPERTY(BlueprintAssignable, Category="Sequence|CutsceneChain")
	FOnCutsceneChainCompleted OnChainCompleted;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool MarkStepInternal(int32 StepIndex);
	bool ValidateStepOrder(int32 StepIndex) const;
	void ApplyInputLock(bool bLock);
	void CompleteChain();

private:
	/** Ordered step list used by index/name APIs. */
	UPROPERTY(EditAnywhere, Category="Sequence|CutsceneChain")
	TArray<FName> StepNames = {
		TEXT("Step01"),
		TEXT("Step02"),
		TEXT("Step03")
	};

	/** true: requires previous steps to be linked first. */
	UPROPERTY(EditAnywhere, Category="Sequence|CutsceneChain")
	bool bRequireOrderedFlow = true;

	/** Lock player input while chain is active. */
	UPROPERTY(EditAnywhere, Category="Sequence|CutsceneChain")
	bool bLockInputDuringChain = false;

	UPROPERTY(EditAnywhere, Category="Sequence|CutsceneChain|Debug")
	bool bEnableChainLog = true;

	UPROPERTY(VisibleAnywhere, Category="Sequence|CutsceneChain")
	bool bChainActive = false;

	UPROPERTY(VisibleAnywhere, Category="Sequence|CutsceneChain")
	bool bChainCompleted = false;

	UPROPERTY(VisibleAnywhere, Category="Sequence|CutsceneChain")
	TArray<bool> StepLinkedStates;

	UPROPERTY()
	TObjectPtr<APlayerController> TargetPlayerController = nullptr;
};
