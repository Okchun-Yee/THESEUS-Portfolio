// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "SplineFollowComponent.generated.h"

class UAnimationAsset;
class USplineComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSplineFollowStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSplineFollowFinished, bool, bInterrupted);

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API USplineFollowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USplineFollowComponent();

    UFUNCTION(BlueprintCallable, Category="Spline|Follow")
    bool StartFollow(USplineComponent* InSplineRef, float InMoveSpeed, bool bInLoop = false);

    UFUNCTION(BlueprintCallable, Category="Spline|Follow")
    void StopFollow();

    UFUNCTION(BlueprintCallable, Category="Spline|Follow")
    bool IsFollowing() const { return bFollowing; }

    UFUNCTION(BlueprintCallable, Category="Spline|Follow")
    void SetSplineRef(USplineComponent* InSplineRef) { SplineRef = InSplineRef; }

    UFUNCTION(BlueprintCallable, Category="Spline|Follow")
    void SetMoveSpeed(float InMoveSpeed) { MoveSpeed = FMath::Max(0.0f, InMoveSpeed); }

    UFUNCTION(BlueprintCallable, Category="Spline|Follow")
    float GetFollowDistance() const { return SplineDistance; }

    UPROPERTY(BlueprintAssignable, Category="Spline|Follow")
    FOnSplineFollowStarted OnFollowStarted;

    UPROPERTY(BlueprintAssignable, Category="Spline|Follow")
    FOnSplineFollowFinished OnFollowFinished;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void UpdateFollow();
    void FinishFollow(bool bInterrupted);
    void UpdateOwnerAnimationPlayback(bool bShouldPlay) const;

private:
    UPROPERTY(EditAnywhere, Category="Spline|Follow")
    TObjectPtr<USplineComponent> SplineRef = nullptr;

    UPROPERTY(EditAnywhere, Category="Spline|Follow", meta=(ClampMin="0.001"))
    float UpdateInterval = 0.02f;

    UPROPERTY(EditAnywhere, Category="Spline|Follow", meta=(ClampMin="0.0"))
    float MoveSpeed = 300.0f;

    UPROPERTY(EditAnywhere, Category="Spline|Follow")
    bool bLoopFollow = false;

    UPROPERTY(EditAnywhere, Category="Spline|Follow")
    bool bAutoStart = false;

    UPROPERTY(EditAnywhere, Category="Spline|Follow")
    bool bAlignRotationToSpline = true;

    UPROPERTY(EditAnywhere, Category="Spline|Follow")
    bool bRestartIfAlreadyFollowing = false;

    UPROPERTY(EditAnywhere, Category="Spline|Follow|Animation")
    bool bAutoPlayOwnerAnimation = false;

    UPROPERTY(EditAnywhere, Category="Spline|Follow|Animation")
    TObjectPtr<UAnimationAsset> FollowAnimation = nullptr;

    UPROPERTY(EditAnywhere, Category="Spline|Follow|Animation")
    bool bLoopOwnerAnimation = true;

    UPROPERTY(EditAnywhere, Category="Spline|Follow|Animation", meta=(ClampMin="0.01"))
    float FollowAnimationPlayRate = 1.0f;

    UPROPERTY(EditAnywhere, Category="Spline|Follow|Animation")
    bool bStopOwnerAnimationOnFinish = true;

    UPROPERTY(VisibleAnywhere, Category="Spline|Follow")
    float SplineDistance = 0.0f;

    UPROPERTY(VisibleAnywhere, Category="Spline|Follow")
    float SplineLength = 0.0f;

    UPROPERTY(VisibleAnywhere, Category="Spline|Follow")
    bool bFollowing = false;

    FTimerHandle FollowTimerHandle;
};
