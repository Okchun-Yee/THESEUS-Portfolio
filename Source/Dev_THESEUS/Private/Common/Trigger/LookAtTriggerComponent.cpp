// Copyright 2026, Okchun-Yee.

#include "Common/Trigger/LookAtTriggerComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

ULookAtTriggerComponent::ULookAtTriggerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULookAtTriggerComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!TargetActor && bAutoAssignPlayerTarget)
    {
        TargetActor = UGameplayStatics::GetPlayerPawn(this, 0);
    }

    if (bAutoStartTracking && TargetActor)
    {
        StartTracking(TargetActor);
    }
}

void ULookAtTriggerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopTracking(false);
    Super::EndPlay(EndPlayReason);
}

bool ULookAtTriggerComponent::StartTracking(AActor* InTargetActor)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LookAtTrigger] StartTracking failed: owner is null."));
        return false;
    }

    if (!InTargetActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LookAtTrigger] StartTracking failed: target is null. Owner=%s"), *Owner->GetName());
        return false;
    }

    if (bTriggerOnce && bTriggered)
    {
        UE_LOG(LogTemp, Log, TEXT("[LookAtTrigger] StartTracking skipped: already triggered and trigger-once is enabled. Owner=%s"), *Owner->GetName());
        return false;
    }

    TargetActor = InTargetActor;
    bTrackingActive = true;
    LoseSightTime = 0.0f;
    bHadLookProgress = AccumulatedLookTime > 0.0f;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EvaluateTimerHandle);
        World->GetTimerManager().SetTimer(
            EvaluateTimerHandle,
            this,
            &ULookAtTriggerComponent::EvaluateLook,
            FMath::Max(CheckInterval, 0.01f),
            true);
    }

    if (bEnableDebugLog)
    {
        UE_LOG(LogTemp, Log, TEXT("[LookAtTrigger] StartTracking: Owner=%s Target=%s DotThreshold=%.2f Required=%.2fs"),
            *Owner->GetName(),
            *InTargetActor->GetName(),
            DotThreshold,
            RequiredLookTime);
    }

    return true;
}

void ULookAtTriggerComponent::StopTracking(bool bResetProgress)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EvaluateTimerHandle);
    }

    bTrackingActive = false;
    LoseSightTime = 0.0f;

    if (bResetProgress)
    {
        AccumulatedLookTime = 0.0f;
        bHadLookProgress = false;
        BroadcastProgress();
    }
}

void ULookAtTriggerComponent::ResetTriggerState(bool bResetProgress)
{
    bTriggered = false;
    RemainingCooldown = 0.0f;

    if (bResetProgress)
    {
        AccumulatedLookTime = 0.0f;
        LoseSightTime = 0.0f;
        bHadLookProgress = false;
        BroadcastProgress();
    }
}

float ULookAtTriggerComponent::GetLookProgress01() const
{
    const float SafeRequired = FMath::Max(RequiredLookTime, 0.01f);
    return FMath::Clamp(AccumulatedLookTime / SafeRequired, 0.0f, 1.0f);
}

void ULookAtTriggerComponent::EvaluateLook()
{
    if (!bTrackingActive)
    {
        return;
    }

    if (bTriggerOnce && bTriggered)
    {
        StopTracking(false);
        return;
    }

    if (!IsValid(GetOwner()))
    {
        if (bEnableDebugLog)
        {
            UE_LOG(LogTemp, Warning, TEXT("[LookAtTrigger] EvaluateLook aborted: owner is invalid."));
        }
        StopTracking(false);
        return;
    }

    if (!IsValid(TargetActor) && bAutoAssignPlayerTarget)
    {
        TargetActor = UGameplayStatics::GetPlayerPawn(this, 0);
        if (bEnableDebugLog && IsValid(TargetActor))
        {
            UE_LOG(LogTemp, Log, TEXT("[LookAtTrigger] Reacquired target: %s"), *TargetActor->GetName());
        }
    }

    if (!IsValid(TargetActor))
    {
        if (bEnableDebugLog)
        {
            UE_LOG(LogTemp, Warning, TEXT("[LookAtTrigger] EvaluateLook skipped: target is invalid."));
        }
        return;
    }

    const float DeltaSeconds = FMath::Max(CheckInterval, 0.01f);
    if (RemainingCooldown > 0.0f)
    {
        RemainingCooldown = FMath::Max(0.0f, RemainingCooldown - DeltaSeconds);
    }

    FVector ViewLocation = FVector::ZeroVector;
    FVector ViewForward = FVector::ForwardVector;
    if (!ResolveViewpoint(ViewLocation, ViewForward))
    {
        return;
    }

    const FVector OwnerLocation = GetOwner()->GetActorLocation();
    FVector DirToOwner = OwnerLocation - ViewLocation;
    if (!DirToOwner.Normalize())
    {
        return;
    }

    const float Dot = FVector::DotProduct(ViewForward.GetSafeNormal(), DirToOwner);
    const bool bPassDot = Dot >= DotThreshold;
    const bool bPassLOS = !bRequireLineOfSight || PassesLineOfSight(ViewLocation, OwnerLocation);
    const bool bLooking = bPassDot && bPassLOS;

    const float PreviousProgress = GetLookProgress01();

    if (bLooking)
    {
        if (!bHadLookProgress)
        {
            OnLookStarted.Broadcast();
        }

        bHadLookProgress = true;
        LoseSightTime = 0.0f;
        AccumulatedLookTime = FMath::Min(AccumulatedLookTime + DeltaSeconds, RequiredLookTime);
    }
    else if (AccumulatedLookTime > 0.0f)
    {
        LoseSightTime += DeltaSeconds;

        if (LoseSightTime >= LoseSightGraceTime)
        {
            if (bUseProgressDecayOnLoseSight)
            {
                AccumulatedLookTime = FMath::Max(0.0f, AccumulatedLookTime - (LoseSightDecayPerSecond * DeltaSeconds));
            }
            else
            {
                AccumulatedLookTime = 0.0f;
            }

            if (AccumulatedLookTime <= KINDA_SMALL_NUMBER)
            {
                AccumulatedLookTime = 0.0f;
                LoseSightTime = 0.0f;
                if (bHadLookProgress)
                {
                    bHadLookProgress = false;
                    OnLookCanceled.Broadcast();
                }
            }
        }
    }

    const float NewProgress = GetLookProgress01();
    if (!FMath::IsNearlyEqual(NewProgress, PreviousProgress, 0.001f))
    {
        BroadcastProgress();
    }

    const bool bCanTrigger = RemainingCooldown <= 0.0f;
    if (!bTriggered && bCanTrigger && AccumulatedLookTime >= RequiredLookTime)
    {
        bTriggered = true;
        OnLookConfirmed.Broadcast();

        if (bEnableDebugLog)
        {
            UE_LOG(LogTemp, Log, TEXT("[LookAtTrigger] Confirmed: Owner=%s Target=%s Dot=%.3f"),
                *GetNameSafe(GetOwner()),
                *GetNameSafe(TargetActor),
                Dot);
        }

        if (bTriggerOnce)
        {
            StopTracking(false);
            return;
        }

        RemainingCooldown = FMath::Max(0.0f, RetriggerCooldownSeconds);
        AccumulatedLookTime = 0.0f;
        LoseSightTime = 0.0f;
        bHadLookProgress = false;
        BroadcastProgress();
        bTriggered = false;
    }
}

bool ULookAtTriggerComponent::ResolveViewpoint(FVector& OutLocation, FVector& OutForward) const
{
    if (!TargetActor)
    {
        return false;
    }

    if (const APawn* TargetPawn = Cast<APawn>(TargetActor.Get()))
    {
        if (const APlayerController* PC = Cast<APlayerController>(TargetPawn->GetController()))
        {
            if (const APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
            {
                const FRotator CameraRot = CameraManager->GetCameraRotation();
                OutLocation = CameraManager->GetCameraLocation();
                OutForward = CameraRot.Vector().GetSafeNormal();
                return true;
            }
        }
    }

    FRotator ViewRotation = FRotator::ZeroRotator;
    TargetActor->GetActorEyesViewPoint(OutLocation, ViewRotation);
    OutForward = ViewRotation.Vector().GetSafeNormal();
    return true;
}

bool ULookAtTriggerComponent::PassesLineOfSight(const FVector& ViewLocation, const FVector& OwnerLocation) const
{
    UWorld* World = GetWorld();
    if (!World || !GetOwner())
    {
        return false;
    }

    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LookAtTriggerLOS), true);
    QueryParams.AddIgnoredActor(TargetActor.Get());
    QueryParams.AddIgnoredActor(GetOwner());

    const bool bHit = World->LineTraceSingleByChannel(
        Hit,
        ViewLocation,
        OwnerLocation,
        LineOfSightTraceChannel,
        QueryParams);

    if (!bHit)
    {
        return true;
    }

    return Hit.GetActor() == GetOwner();
}

void ULookAtTriggerComponent::BroadcastProgress()
{
    const float CurrentProgress = GetLookProgress01();
    if (FMath::IsNearlyEqual(LastBroadcastProgress, CurrentProgress, 0.001f))
    {
        return;
    }

    LastBroadcastProgress = CurrentProgress;
    OnLookProgress.Broadcast(CurrentProgress);
}
