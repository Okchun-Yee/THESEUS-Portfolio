// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "BreakableLightComponent.generated.h"

class ULightComponent;
class UMeshComponent;
class ULightFlickerModeComponent;
class USoundBase;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTheseusLightBreakSequenceStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTheseusLightBroken);

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API UBreakableLightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBreakableLightComponent();

	/** Starts optional pre-break flicker and then final break. Returns false if already broken/in progress. */
	UFUNCTION(BlueprintCallable, Category="Light|Break")
	bool StartBreakSequence();

	/** Immediately applies final broken state. Safe to call multiple times. */
	UFUNCTION(BlueprintCallable, Category="Light|Break")
	void BreakLightFinal();

	/** Resets broken state for editor/runtime re-test. */
	UFUNCTION(BlueprintCallable, Category="Light|Break")
	void ResetBreakState(bool bRestoreLightState = true);

	UFUNCTION(BlueprintCallable, Category="Light|Break")
	bool IsBroken() const { return bIsBroken; }

	UFUNCTION(BlueprintCallable, Category="Light|Break")
	bool IsBreakInProgress() const { return bBreakInProgress; }

	UPROPERTY(BlueprintAssignable, Category="Light|Break")
	FOnTheseusLightBreakSequenceStarted OnBreakSequenceStarted;

	UPROPERTY(BlueprintAssignable, Category="Light|Break")
	FOnTheseusLightBroken OnLightBroken;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ResolveTargetsIfNeeded();
	void ApplyFinalBrokenState();
	void ScheduleBrokenMeshPhysics();
	void ApplyBrokenMeshPhysics();

private:
	UPROPERTY(EditAnywhere, Category="Light|Break|Target")
	TObjectPtr<ULightComponent> TargetLight = nullptr;

	UPROPERTY(EditAnywhere, Category="Light|Break|Target")
	TObjectPtr<ULightFlickerModeComponent> FlickerComponent = nullptr;

	UPROPERTY(EditAnywhere, Category="Light|Break|Target")
	bool bUseMeshSwap = false;

	UPROPERTY(EditAnywhere, Category="Light|Break|Target", meta=(EditCondition="bUseMeshSwap"))
	FName IntactMeshTag = TEXT("Light_Intact");

	UPROPERTY(EditAnywhere, Category="Light|Break|Target", meta=(EditCondition="bUseMeshSwap"))
	FName BrokenMeshTag = TEXT("Light_Broken");

	UPROPERTY(EditAnywhere, Category="Light|Break|Target", meta=(EditCondition="bUseMeshSwap"))
	bool bUseDelayedBrokenMeshPhysics = false;

	UPROPERTY(EditAnywhere, Category="Light|Break|Target", meta=(EditCondition="bUseMeshSwap && bUseDelayedBrokenMeshPhysics", ClampMin="0.0"))
	float BrokenMeshPhysicsDelayMinSeconds = 0.5f;

	UPROPERTY(EditAnywhere, Category="Light|Break|Target", meta=(EditCondition="bUseMeshSwap && bUseDelayedBrokenMeshPhysics", ClampMin="0.0"))
	float BrokenMeshPhysicsDelayMaxSeconds = 0.8f;

	UPROPERTY(EditAnywhere, Category="Light|Break|Setup")
	bool bAutoResolveTargets = true;

	UPROPERTY(EditAnywhere, Category="Light|Break|Setup")
	bool bUsePreBreakFlicker = true;

	UPROPERTY(EditAnywhere, Category="Light|Break|Setup", meta=(ClampMin="1"))
	int32 PreBreakBlinkCount = 4;

	UPROPERTY(EditAnywhere, Category="Light|Break|Setup", meta=(ClampMin="0.01"))
	float PreBreakIntervalSeconds = 0.06f;

	UPROPERTY(EditAnywhere, Category="Light|Break|Setup", meta=(ClampMin="0.0"))
	float PreBreakDelaySeconds = 0.20f;

	UPROPERTY(EditAnywhere, Category="Light|Break|Setup")
	bool bBlockFlickerStartAfterBreak = true;

	UPROPERTY(EditAnywhere, Category="Light|Break|VFX")
	TObjectPtr<UNiagaraSystem> BreakFx = nullptr;

	UPROPERTY(EditAnywhere, Category="Light|Break|VFX")
	FVector BreakFxOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="Light|Break|SFX")
	TObjectPtr<USoundBase> BreakSound = nullptr;

	UPROPERTY(EditAnywhere, Category="Light|Break|Debug")
	bool bEnableDebugLog = false;

	UPROPERTY(VisibleAnywhere, Category="Light|Break|Runtime")
	bool bIsBroken = false;

	UPROPERTY(VisibleAnywhere, Category="Light|Break|Runtime")
	bool bBreakInProgress = false;

	UPROPERTY(Transient)
	float CachedOriginalIntensity = 0.0f;

	UPROPERTY(Transient)
	bool bCachedOriginalVisibility = true;

	UPROPERTY(Transient)
	bool bCachedIntactMeshVisibility = true;

	UPROPERTY(Transient)
	bool bCachedBrokenMeshVisibility = false;

	UPROPERTY(Transient)
	bool bCachedBrokenMeshSimulatePhysics = false;

	UPROPERTY(Transient)
	bool bCachedBrokenMeshEnableGravity = true;

	UPROPERTY(Transient)
	TObjectPtr<UMeshComponent> ResolvedIntactMesh = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMeshComponent> ResolvedBrokenMesh = nullptr;

	FTimerHandle BreakSequenceTimerHandle;
	FTimerHandle BrokenMeshPhysicsTimerHandle;
};
