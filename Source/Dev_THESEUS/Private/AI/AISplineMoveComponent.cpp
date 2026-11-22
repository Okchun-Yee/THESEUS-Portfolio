// Copyright 2026, Okchun-Yee.

#include "AI/AISplineMoveComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/UnrealType.h"

UAISplineMoveComponent::UAISplineMoveComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAISplineMoveComponent::BeginPlay()
{
    Super::BeginPlay();

    // Default AI state: cannot receive point damage until spline sequence starts.
    if (!SetOwnerReceivePointDamageEnabled(false))
    {
        UE_LOG(LogTemp, Warning, TEXT("[AISplineMove] BeginPlay: failed to set 'Recieve Damage Point' to false. Owner=%s"), *GetNameSafe(GetOwner()));
    }
}

bool UAISplineMoveComponent::StartSplineMove(USplineComponent* InSplineRef, float InMoveSpeed, bool bInLoop)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AISplineMove] StartSplineMove failed: owner is null."));
        return false;
    }

    if (!InSplineRef)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AISplineMove] StartSplineMove failed: spline ref is null. Owner=%s"), *Owner->GetName());
        return false;
    }

    if (InMoveSpeed <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AISplineMove] StartSplineMove failed: speed must be > 0. Owner=%s Speed=%.2f"), *Owner->GetName(), InMoveSpeed);
        return false;
    }

    if (bSplineMoving)
    {
        if (!bRestartIfAlreadyMoving)
        {
            UE_LOG(LogTemp, Warning, TEXT("[AISplineMove] StartSplineMove skipped: already moving and restart disabled. Owner=%s"), *Owner->GetName());
            return false;
        }

        UE_LOG(LogTemp, Log, TEXT("[AISplineMove] StartSplineMove restart requested while moving. Owner=%s"), *Owner->GetName());
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SplineUpdateTimerHandle);
    }

    SplineRef = InSplineRef;
    MoveSpeed = InMoveSpeed;
    bLoopSpline = bInLoop;
    SplineDistance = 0.0f;
    SplineLength = SplineRef->GetSplineLength();
    bSplineMoving = SplineLength > KINDA_SMALL_NUMBER;

    if (!bSplineMoving)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AISplineMove] StartSplineMove failed: spline length is too short."));
        return false;
    }

    if (!SetOwnerReceivePointDamageEnabled(true))
    {
        UE_LOG(LogTemp, Warning, TEXT("[AISplineMove] StartSplineMove: failed to set 'Recieve Damage Point' to true. Owner=%s"), *Owner->GetName());
    }

    if (ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
    {
        if (UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement())
        {
            MoveComp->StopMovementImmediately();
            MoveComp->DisableMovement();
        }
    }

    const FVector StartLocation = SplineRef->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
    const FRotator StartRotation = SplineRef->GetRotationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
    Owner->SetActorLocationAndRotation(StartLocation, StartRotation);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            SplineUpdateTimerHandle,
            this,
            &UAISplineMoveComponent::UpdateSplineMove,
            FMath::Max(UpdateInterval, 0.001f),
            true);
    }

    UE_LOG(LogTemp, Log, TEXT("[AISplineMove] StartSplineMove: Spline=%s Speed=%.2f Loop=%s Length=%.2f"),
        *SplineRef->GetName(),
        MoveSpeed,
        bLoopSpline ? TEXT("true") : TEXT("false"),
        SplineLength);
    return true;
}

TSubclassOf<AActor> UAISplineMoveComponent::SelectGrenadeClass() const
{
    TArray<TSubclassOf<AActor>> ValidGrenadeClasses;
    ValidGrenadeClasses.Reserve(GrenadeClasses.Num());
    for (const TSubclassOf<AActor>& CandidateClass : GrenadeClasses)
    {
        if (CandidateClass)
        {
            ValidGrenadeClasses.Add(CandidateClass);
        }
    }

    if (ValidGrenadeClasses.Num() > 0)
    {
        const int32 RandomIndex = FMath::RandRange(0, ValidGrenadeClasses.Num() - 1);
        return ValidGrenadeClasses[RandomIndex];
    }

    return GrenadeClassFallback;
}

AActor* UAISplineMoveComponent::SpawnRandomGrenadeAtOwner()
{
    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();
    if (!Owner || !World)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AISplineMove] SpawnRandomGrenadeAtOwner skipped: owner/world is invalid."));
        return nullptr;
    }

    TSubclassOf<AActor> GrenadeClass = SelectGrenadeClass();
    if (!GrenadeClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AISplineMove] SpawnRandomGrenadeAtOwner skipped: no valid grenade class configured. Owner=%s"), *Owner->GetName());
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = GrenadeSpawnCollisionHandling;
    SpawnParams.Owner = Owner;

    AActor* SpawnedGrenade = World->SpawnActor<AActor>(GrenadeClass, Owner->GetActorTransform(), SpawnParams);
    if (!SpawnedGrenade)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AISplineMove] SpawnRandomGrenadeAtOwner failed. Owner=%s Class=%s"), *Owner->GetName(), *GrenadeClass->GetName());
        return nullptr;
    }

    UE_LOG(LogTemp, Log, TEXT("[AISplineMove] SpawnRandomGrenadeAtOwner: Owner=%s Class=%s Spawned=%s"),
        *Owner->GetName(),
        *GrenadeClass->GetName(),
        *SpawnedGrenade->GetName());
    return SpawnedGrenade;
}

void UAISplineMoveComponent::StopSplineMove()
{
    FinishSplineMove(true);
}

void UAISplineMoveComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FinishSplineMove(true);
    Super::EndPlay(EndPlayReason);
}

void UAISplineMoveComponent::UpdateSplineMove()
{
    if (!bSplineMoving || !SplineRef || !GetOwner())
    {
        FinishSplineMove(true);
        return;
    }

    const float Step = MoveSpeed * FMath::Max(UpdateInterval, 0.001f);
    SplineDistance += Step;

    if (SplineDistance >= SplineLength)
    {
        if (bLoopSpline)
        {
            SplineDistance = FMath::Fmod(SplineDistance, SplineLength);
        }
        else
        {
            SplineDistance = SplineLength;
            const FVector EndLocation = SplineRef->GetLocationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World);
            const FRotator EndRotation = FRotator(0.0f, SplineRef->GetRotationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World).Yaw, 0.0f);
            GetOwner()->SetActorLocationAndRotation(EndLocation, EndRotation);
            FinishSplineMove(false);
            return;
        }
    }

    const FVector NewLocation = SplineRef->GetLocationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World);
    const FRotator NewRotation = FRotator(0.0f, SplineRef->GetRotationAtDistanceAlongSpline(SplineDistance, ESplineCoordinateSpace::World).Yaw, 0.0f);
    GetOwner()->SetActorLocationAndRotation(NewLocation, NewRotation);
}

void UAISplineMoveComponent::FinishSplineMove(bool bInterrupted)
{
    const bool bWasMoving = bSplineMoving;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SplineUpdateTimerHandle);
    }

    if (bSplineMoving)
    {
        UE_LOG(LogTemp, Log, TEXT("[AISplineMove] FinishSplineMove: Interrupted=%s Distance=%.2f/%.2f"),
            bInterrupted ? TEXT("true") : TEXT("false"),
            SplineDistance,
            SplineLength);
    }

    bSplineMoving = false;

    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement())
        {
            MoveComp->SetMovementMode(MOVE_Walking);
        }
    }

    if (bWasMoving)
    {
        OnSplineMoveFinished.Broadcast(bInterrupted);
    }
}

bool UAISplineMoveComponent::SetOwnerReceivePointDamageEnabled(bool bEnabled) const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    static const FName CandidatePropertyNames[] = {
        TEXT("RecieveDamagePoint"),
        TEXT("ReceiveDamagePoint"),
        TEXT("bRecieveDamagePoint"),
        TEXT("bReceiveDamagePoint"),
        TEXT("Recieve_Damage_Point"),
        TEXT("Receive_Damage_Point")
    };

    TArray<UActorComponent*> Components;
    Owner->GetComponents(Components);

    for (UActorComponent* Component : Components)
    {
        if (!IsValid(Component))
        {
            continue;
        }

        FBoolProperty* BoolProp = nullptr;
        for (const FName CandidateName : CandidatePropertyNames)
        {
            BoolProp = FindFProperty<FBoolProperty>(Component->GetClass(), CandidateName);
            if (BoolProp)
            {
                break;
            }
        }

        if (!BoolProp)
        {
            auto NormalizeBoolName = [](const FString& InName) -> FString
            {
                FString Out;
                Out.Reserve(InName.Len());
                for (const TCHAR Ch : InName)
                {
                    if (FChar::IsAlnum(Ch))
                    {
                        Out.AppendChar(FChar::ToLower(Ch));
                    }
                }
                return Out;
            };

            static const FString RecieveDamagePointKey = TEXT("recievedamagepoint");
            static const FString ReceiveDamagePointKey = TEXT("receivedamagepoint");

            for (TFieldIterator<FBoolProperty> It(Component->GetClass()); It; ++It)
            {
                FBoolProperty* CandidateProp = *It;
                if (!CandidateProp)
                {
                    continue;
                }

                const FString NormalizedName = NormalizeBoolName(CandidateProp->GetName());
                if (NormalizedName == RecieveDamagePointKey || NormalizedName == ReceiveDamagePointKey)
                {
                    BoolProp = CandidateProp;
                    break;
                }
            }
        }

        if (!BoolProp)
        {
            continue;
        }

        BoolProp->SetPropertyValue_InContainer(Component, bEnabled);
        UE_LOG(LogTemp, Log, TEXT("[AISplineMove] Set '%s'=%s on %s (Owner=%s)"),
            *BoolProp->GetName(),
            bEnabled ? TEXT("true") : TEXT("false"),
            *Component->GetName(),
            *Owner->GetName());
        return true;
    }

    return false;
}
