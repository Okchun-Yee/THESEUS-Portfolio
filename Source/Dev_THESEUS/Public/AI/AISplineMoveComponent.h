// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "AISplineMoveComponent.generated.h"

class AActor;
class USplineComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAISplineMoveFinished, bool, bInterrupted);

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API UAISplineMoveComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAISplineMoveComponent();

    UFUNCTION(BlueprintCallable, Category="AI|Spline")
    bool StartSplineMove(USplineComponent* InSplineRef, float InMoveSpeed, bool bInLoop = false);

    UFUNCTION(BlueprintCallable, Category="AI|Spline")
    void StopSplineMove();

    UFUNCTION(BlueprintCallable, Category="AI|Spline")
    bool IsSplineMoving() const { return bSplineMoving; }

    UFUNCTION(BlueprintCallable, Category="AI|Spline")
    float GetSplineDistance() const { return SplineDistance; }

    UFUNCTION(BlueprintCallable, Category="AI|Spline")
    float GetMoveSpeed() const { return MoveSpeed; }

    /** Returns one grenade class using random selection from GrenadeClasses, with GrenadeClassFallback as fallback. */
    UFUNCTION(BlueprintCallable, Category="AI|Spline|Grenade")
    TSubclassOf<AActor> SelectGrenadeClass() const;

    /** Spawns one grenade actor with random class selection at the owner's current transform. */
    UFUNCTION(BlueprintCallable, Category="AI|Spline|Grenade")
    AActor* SpawnRandomGrenadeAtOwner();

    UPROPERTY(BlueprintAssignable, Category="AI|Spline")
    FOnAISplineMoveFinished OnSplineMoveFinished;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void UpdateSplineMove();
    void FinishSplineMove(bool bInterrupted);
    bool SetOwnerReceivePointDamageEnabled(bool bEnabled) const;

private:
    UPROPERTY()
    TObjectPtr<USplineComponent> SplineRef = nullptr;

    UPROPERTY(EditAnywhere, Category="AI|Spline", meta=(ClampMin="0.001"))
    float UpdateInterval = 0.02f;

    /** true면 이동 중 동일 요청이 들어올 때 기존 이동을 재시작한다. */
    UPROPERTY(EditAnywhere, Category="AI|Spline")
    bool bRestartIfAlreadyMoving = true;

    UPROPERTY(VisibleAnywhere, Category="AI|Spline")
    float SplineDistance = 0.0f;

    UPROPERTY(VisibleAnywhere, Category="AI|Spline")
    float SplineLength = 0.0f;

    UPROPERTY(VisibleAnywhere, Category="AI|Spline")
    float MoveSpeed = 300.0f;

    UPROPERTY(VisibleAnywhere, Category="AI|Spline")
    bool bSplineMoving = false;

    UPROPERTY(VisibleAnywhere, Category="AI|Spline")
    bool bLoopSpline = false;

    /** Candidate grenade actor classes for random selection. */
    UPROPERTY(EditAnywhere, Category="AI|Spline|Grenade")
    TArray<TSubclassOf<AActor>> GrenadeClasses;

    /** Fallback grenade class used when GrenadeClasses has no valid entries. */
    UPROPERTY(EditAnywhere, Category="AI|Spline|Grenade")
    TSubclassOf<AActor> GrenadeClassFallback;

    UPROPERTY(EditAnywhere, Category="AI|Spline|Grenade")
    ESpawnActorCollisionHandlingMethod GrenadeSpawnCollisionHandling = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    FTimerHandle SplineUpdateTimerHandle;
};
