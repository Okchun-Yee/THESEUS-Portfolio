// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "LookAtTriggerComponent.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLookTriggerSimple);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLookTriggerProgress, float, Progress01);

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API ULookAtTriggerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULookAtTriggerComponent();

    UFUNCTION(BlueprintCallable, Category="LookTrigger")
    bool StartTracking(AActor* InTargetActor);

    UFUNCTION(BlueprintCallable, Category="LookTrigger")
    void StopTracking(bool bResetProgress = true);

    UFUNCTION(BlueprintCallable, Category="LookTrigger")
    void ResetTriggerState(bool bResetProgress = true);

    UFUNCTION(BlueprintCallable, Category="LookTrigger")
    bool IsTracking() const { return bTrackingActive; }

    UFUNCTION(BlueprintCallable, Category="LookTrigger")
    bool HasTriggered() const { return bTriggered; }

    UFUNCTION(BlueprintCallable, Category="LookTrigger")
    void SetTargetActor(AActor* InTargetActor) { TargetActor = InTargetActor; }

    UFUNCTION(BlueprintCallable, Category="LookTrigger")
    AActor* GetTargetActor() const { return TargetActor.Get(); }

    UFUNCTION(BlueprintCallable, Category="LookTrigger")
    float GetLookProgress01() const;

    UPROPERTY(BlueprintAssignable, Category="LookTrigger")
    FOnLookTriggerSimple OnLookStarted;

    UPROPERTY(BlueprintAssignable, Category="LookTrigger")
    FOnLookTriggerProgress OnLookProgress;

    UPROPERTY(BlueprintAssignable, Category="LookTrigger")
    FOnLookTriggerSimple OnLookCanceled;

    UPROPERTY(BlueprintAssignable, Category="LookTrigger")
    FOnLookTriggerSimple OnLookConfirmed;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void EvaluateLook();
    bool ResolveViewpoint(FVector& OutLocation, FVector& OutForward) const;
    bool PassesLineOfSight(const FVector& ViewLocation, const FVector& OwnerLocation) const;
    void BroadcastProgress();

private:
    UPROPERTY(EditAnywhere, Category="LookTrigger|Setup")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Setup")
    bool bAutoAssignPlayerTarget = true;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Setup")
    bool bAutoStartTracking = true;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Condition", meta=(ClampMin="-1.0", ClampMax="1.0"))
    float DotThreshold = 0.90f;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Condition", meta=(ClampMin="0.01"))
    float RequiredLookTime = 2.0f;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Condition", meta=(ClampMin="0.0"))
    float LoseSightGraceTime = 0.15f;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Condition")
    bool bUseProgressDecayOnLoseSight = false;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Condition", meta=(ClampMin="0.0"))
    float LoseSightDecayPerSecond = 2.0f;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Condition")
    bool bRequireLineOfSight = false;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Condition")
    TEnumAsByte<ECollisionChannel> LineOfSightTraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Trigger")
    bool bTriggerOnce = true;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Trigger", meta=(ClampMin="0.0"))
    float RetriggerCooldownSeconds = 0.0f;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Runtime", meta=(ClampMin="0.01"))
    float CheckInterval = 0.05f;

    UPROPERTY(EditAnywhere, Category="LookTrigger|Debug")
    bool bEnableDebugLog = false;

    UPROPERTY(VisibleAnywhere, Category="LookTrigger|Runtime")
    bool bTrackingActive = false;

    UPROPERTY(VisibleAnywhere, Category="LookTrigger|Runtime")
    bool bTriggered = false;

    UPROPERTY(VisibleAnywhere, Category="LookTrigger|Runtime")
    float AccumulatedLookTime = 0.0f;

    UPROPERTY(VisibleAnywhere, Category="LookTrigger|Runtime")
    float LoseSightTime = 0.0f;

    UPROPERTY(VisibleAnywhere, Category="LookTrigger|Runtime")
    float RemainingCooldown = 0.0f;

    bool bHadLookProgress = false;
    float LastBroadcastProgress = -1.0f;
    FTimerHandle EvaluateTimerHandle;
};
