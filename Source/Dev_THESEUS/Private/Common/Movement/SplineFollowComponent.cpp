// Copyright 2026, Okchun-Yee.

#include "Common/Movement/SplineFollowComponent.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

USplineFollowComponent::USplineFollowComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USplineFollowComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoStart && SplineRef)
    {
        StartFollow(SplineRef, MoveSpeed, bLoopFollow);
    }
}

bool USplineFollowComponent::StartFollow(USplineComponent* InSplineRef, float InMoveSpeed, bool bInLoop)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SplineFollow] StartFollow failed: owner is null."));
        return false;
    }

    if (!InSplineRef)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SplineFollow] StartFollow failed: spline ref is null. Owner=%s"), *Owner->GetName());
        return false;
    }

    if (InMoveSpeed <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SplineFollow] StartFollow failed: speed must be > 0. Owner=%s Speed=%.2f"), *Owner->GetName(), InMoveSpeed);
        return false;
    }

    if (bFollowing && !bRestartIfAlreadyFollowing)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SplineFollow] StartFollow skipped: already following and restart disabled. Owner=%s"), *Owner->GetName());
        return false;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FollowTimerHandle);
    }

    SplineRef = InSplineRef;
    MoveSpeed = InMoveSpeed;
    bLoopFollow = bInLoop;
    SplineDistance = 0.0f;
    SplineLength = SplineRef->GetSplineLength();
    bFollowing = SplineLength > KINDA_SMALL_NUMBER;

    if (!bFollowing)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SplineFollow] StartFollow failed: spline length is too short."));
        return false;
    }

    const FVector StartLocation = SplineRef->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
    const FRotator StartRotation = bAlignRotationToSpline
        ? SplineRef->GetRotationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World)
        : Owner->GetActorRotation();
    Owner->SetActorLocationAndRotation(StartLocation, StartRotation);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            FollowTimerHandle,
            this,
            &USplineFollowComponent::UpdateFollow,
            FMath::Max(UpdateInterval, 0.001f),
            true);
    }

    UpdateOwnerAnimationPlayback(true);
    OnFollowStarted.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("[SplineFollow] StartFollow: Spline=%s Speed=%.2f Loop=%s Length=%.2f"),
        *SplineRef->GetName(),
        MoveSpeed,
        bLoopFollow ? TEXT("true") : TEXT("false"),
        SplineLength);

    return true;
}

void USplineFollowComponent::StopFollow()
{
    FinishFollow(true);
}

void USplineFollowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FinishFollow(true);
    Super::EndPlay(EndPlayReason);
}

void USplineFollowComponent::UpdateFollow()
{
    AActor* Owner = GetOwner();
    if (!bFollowing || !SplineRef || !Owner)
    {
        FinishFollow(true);
        return;
    }

    const float Step = MoveSpeed * FMath::Max(UpdateInterval, 0.001f);
    SplineDistance += Step;

    if (SplineDistance >= SplineLength)
    {
        if (bLoopFollow)
        {
            SplineDistance = FMath::Fmod(SplineDistance, SplineLength);
        }
        else
        {
            SplineDistance = SplineLength;
            const FVector EndLocation = SplineRef->GetLocationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World);
            const FRotator EndRotation = bAlignRotationToSpline
                ? SplineRef->GetRotationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World)
                : Owner->GetActorRotation();
            Owner->SetActorLocationAndRotation(EndLocation, EndRotation);
            FinishFollow(false);
            return;
        }
    }

    const FVector NewLocation = SplineRef->GetLocationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World);
    const FRotator NewRotation = bAlignRotationToSpline
        ? SplineRef->GetRotationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World)
        : Owner->GetActorRotation();
    Owner->SetActorLocationAndRotation(NewLocation, NewRotation);
}

void USplineFollowComponent::FinishFollow(bool bInterrupted)
{
    const bool bWasFollowing = bFollowing;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FollowTimerHandle);
    }

    if (bFollowing)
    {
        UE_LOG(LogTemp, Log, TEXT("[SplineFollow] FinishFollow: Interrupted=%s Distance=%.2f/%.2f"),
            bInterrupted ? TEXT("true") : TEXT("false"),
            SplineDistance,
            SplineLength);
    }

    bFollowing = false;

    if (bWasFollowing)
    {
        UpdateOwnerAnimationPlayback(false);
        OnFollowFinished.Broadcast(bInterrupted);
    }
}

void USplineFollowComponent::UpdateOwnerAnimationPlayback(bool bShouldPlay) const
{
    if (!bAutoPlayOwnerAnimation)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    USkeletalMeshComponent* SkeletalMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
    if (!SkeletalMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SplineFollow] Animation playback skipped: no skeletal mesh. Owner=%s"), *Owner->GetName());
        return;
    }

    if (bShouldPlay)
    {
        if (!FollowAnimation)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SplineFollow] Animation playback skipped: FollowAnimation is null. Owner=%s"), *Owner->GetName());
            return;
        }

        SkeletalMesh->PlayAnimation(FollowAnimation, bLoopOwnerAnimation);
        SkeletalMesh->SetPlayRate(FMath::Max(0.01f, FollowAnimationPlayRate));
        return;
    }

    if (bStopOwnerAnimationOnFinish)
    {
        SkeletalMesh->Stop();
    }
}

