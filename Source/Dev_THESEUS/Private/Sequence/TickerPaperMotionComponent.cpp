// Copyright 2026, Okchun-Yee.


#include "Sequence/TickerPaperMotionComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UTickerPaperMotionComponent::UTickerPaperMotionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UTickerPaperMotionComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UTickerPaperMotionComponent::InitPaper(
    UPrimitiveComponent* InPaperPrimitive,
    const FTickerPrintRequest& InRequest,
    const FVector& InStart,
    const FVector& InForward,
    const FRotator& InStartRotation,
    USceneComponent* InFollowSourceComponent)
{
    Activate(true);
    SetComponentTickEnabled(true);

    PaperPrimitive = InPaperPrimitive;
    CachedRequest = InRequest;
    StartLocation = InStart;
    ForwardDir = InForward.GetSafeNormal();
    StartRotation = InStartRotation;
    FollowSourceComponent = InFollowSourceComponent;
    EjectElapsed = 0.0f;
    bPhysicsEnabled = false;
    bFluttering = false;
    PhysicsEnabledAtTime = -1.0f;

    if (!PaperPrimitive)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TickerPaperMotion] InitPaper failed: PaperPrimitive is null."));
        return;
    }

    PaperPrimitive->SetSimulatePhysics(false);
    PaperPrimitive->SetEnableGravity(false);
    PaperPrimitive->SetWorldLocation(StartLocation);
    PaperPrimitive->SetWorldRotation(StartRotation);

    PaperPrimitive->OnComponentHit.RemoveDynamic(this, &UTickerPaperMotionComponent::HandlePaperHit);
    PaperPrimitive->OnComponentHit.AddDynamic(this, &UTickerPaperMotionComponent::HandlePaperHit);

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(LifeTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(EjectFailSafeTimerHandle);

        if (CachedRequest.LifeTime > 0.0f)
        {
            GetWorld()->GetTimerManager().SetTimer(
                LifeTimerHandle,
                this,
                &UTickerPaperMotionComponent::DestroyOwner,
                CachedRequest.LifeTime,
                false);
        }

        const float FailSafeDelay = FMath::Max(EjectDuration + 0.05f, 0.05f);
        GetWorld()->GetTimerManager().SetTimer(
            EjectFailSafeTimerHandle,
            this,
            &UTickerPaperMotionComponent::EnablePhysicsPhase,
            FailSafeDelay,
            false);
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[TickerPaperMotion] InitPaper: Message=\"%s\" EjectDistance=%.2f LifeTime=%.2f Start=%s Forward=%s FollowSource=%s"),
        *CachedRequest.Message.ToString(),
        CachedRequest.EjectDistance,
        CachedRequest.LifeTime,
        *StartLocation.ToCompactString(),
        *ForwardDir.ToCompactString(),
        FollowSourceComponent.IsValid() ? *FollowSourceComponent->GetName() : TEXT("None"));
}

void UTickerPaperMotionComponent::StartFlutter()
{
    bFluttering = true;
    UE_LOG(LogTemp, Log, TEXT("[TickerPaperMotion] StartFlutter."));
}

void UTickerPaperMotionComponent::StopFlutter()
{
    bFluttering = false;
    UE_LOG(LogTemp, Log, TEXT("[TickerPaperMotion] StopFlutter."));

    if (PaperPrimitive && PaperPrimitive->IsSimulatingPhysics())
    {
        PaperPrimitive->SetLinearDamping(3.0f);
        PaperPrimitive->SetAngularDamping(4.0f);
    }
}

void UTickerPaperMotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!PaperPrimitive)
    {
        return;
    }

    if (!bPhysicsEnabled)
    {
        EjectElapsed += DeltaTime;
        const float Alpha = (EjectDuration <= KINDA_SMALL_NUMBER) ? 1.0f : FMath::Clamp(EjectElapsed / EjectDuration, 0.0f, 1.0f);

        FVector CurrentStart = StartLocation;
        FVector CurrentForward = ForwardDir;
        FRotator CurrentRotation = StartRotation;
        if (USceneComponent* FollowSource = FollowSourceComponent.Get())
        {
            CurrentStart = FollowSource->GetComponentLocation();
            CurrentForward = FollowSource->GetForwardVector().GetSafeNormal();
            CurrentRotation = FollowSource->GetComponentRotation();
        }

        const FVector End = CurrentStart + CurrentForward * CachedRequest.EjectDistance;
        PaperPrimitive->SetWorldLocation(FMath::Lerp(CurrentStart, End, Alpha));
        PaperPrimitive->SetWorldRotation(CurrentRotation);

        if (Alpha >= 1.0f)
        {
            EnablePhysicsPhase();
        }

        return;
    }

    if (bFluttering && PaperPrimitive->IsSimulatingPhysics())
    {
        const float TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        const FVector Side = FVector::CrossProduct(ForwardDir, FVector::UpVector).GetSafeNormal();
        const float Wave = FMath::Sin(TimeSeconds * FlutterFrequency);

        PaperPrimitive->AddForce((FVector::UpVector * 0.45f + Side * Wave) * FlutterForce, NAME_None, true);
        PaperPrimitive->AddTorqueInRadians(Side * (FlutterTorque * Wave), NAME_None, true);
    }
}

void UTickerPaperMotionComponent::HandlePaperHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (!bPhysicsEnabled)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        if (PhysicsEnabledAtTime >= 0.0f && (World->GetTimeSeconds() - PhysicsEnabledAtTime) < HitIgnoreTimeAfterPhysics)
        {
            return;
        }
    }

    if (!bFluttering)
    {
        return;
    }

    AActor* InstigatorActor = CachedRequest.InstigatorActor;
    AActor* InstigatorOwner = InstigatorActor ? InstigatorActor->GetOwner() : nullptr;
    APawn* InstigatorPawn = InstigatorActor ? InstigatorActor->GetInstigator() : nullptr;

    if (OtherActor && (
        OtherActor == GetOwner() ||
        OtherActor == InstigatorActor ||
        OtherActor == InstigatorOwner ||
        OtherActor == InstigatorPawn))
    {
        return;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[TickerPaperMotion] Hit: OtherActor=%s ImpactPoint=%s Normal=%s"),
        OtherActor ? *OtherActor->GetName() : TEXT("None"),
        *Hit.ImpactPoint.ToCompactString(),
        *Hit.ImpactNormal.ToCompactString());

    StopFlutter();

    if (PaperPrimitive)
    {
        PaperPrimitive->SetNotifyRigidBodyCollision(false);
        PaperPrimitive->OnComponentHit.RemoveDynamic(this, &UTickerPaperMotionComponent::HandlePaperHit);
    }
}

void UTickerPaperMotionComponent::EnablePhysicsPhase()
{
    if (bPhysicsEnabled)
    {
        return;
    }

    if (!PaperPrimitive)
    {
        return;
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(EjectFailSafeTimerHandle);
    }

    bPhysicsEnabled = true;
    PhysicsEnabledAtTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f;

    PaperPrimitive->SetEnableGravity(true);
    PaperPrimitive->SetSimulatePhysics(true);
    PaperPrimitive->SetNotifyRigidBodyCollision(true);
    PaperPrimitive->SetLinearDamping(0.2f);
    PaperPrimitive->SetAngularDamping(0.3f);

    UE_LOG(LogTemp, Log, TEXT("[TickerPaperMotion] EnablePhysicsPhase: SimulatePhysics=true."));

    StartFlutter();
}

void UTickerPaperMotionComponent::DestroyOwner()
{
    if (AActor* Owner = GetOwner())
    {
        UE_LOG(LogTemp, Log, TEXT("[TickerPaperMotion] DestroyOwner: %s"), *Owner->GetName());
        Owner->Destroy();
    }
}
