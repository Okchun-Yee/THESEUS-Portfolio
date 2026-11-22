// Copyright 2026, Okchun-Yee.


#include "Sequence/TickerPaperManager.h"
#include "Animation/AnimInstance.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Sequence/TheseusEventBusSubsystem.h"
#include "Sequence/TickerPaperActor.h"

// Sets default values
ATickerPaperManager::ATickerPaperManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ATickerPaperManager::BeginPlay()
{
	Super::BeginPlay();
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UTheseusEventBusSubsystem* Bus = GI->GetSubsystem<UTheseusEventBusSubsystem>())
        {
            CachedBus = Bus;
            Bus->OnTickerPrintRequested.AddDynamic(this, &ATickerPaperManager::HandleTickerPrintRequested);

            UE_LOG(LogTemp, Log, TEXT("[TickerPaperManager] Subscribed to EventBus."));
        }
    }
}

void ATickerPaperManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UTheseusEventBusSubsystem* Bus = CachedBus.Get())
    {
        Bus->OnTickerPrintRequested.RemoveDynamic(this, &ATickerPaperManager::HandleTickerPrintRequested);
        UE_LOG(LogTemp, Log, TEXT("[TickerPaperManager] Unsubscribed from EventBus."));
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MontagePlayTimerHandle);
    }

    ApplyTickerInputLock(false);

    Super::EndPlay(EndPlayReason);
}

void ATickerPaperManager::HandleTickerPrintRequested(const FTickerPrintRequest& Request)
{
    if (!TickerPaperClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TickerPaperManager] TickerPaperClass is null."));
        return;
    }

    // 1. 몽타주 즉시 재생
    if (PrintMontage)
    {
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                TArray<USkeletalMeshComponent*> Meshes;
                Pawn->GetComponents<USkeletalMeshComponent>(Meshes);

                for (USkeletalMeshComponent* Mesh : Meshes)
                {
                    if (!IsValid(Mesh) || Mesh->GetFName() != ArmsMeshComponentName)
                    {
                        continue;
                    }

                    if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
                    {
                        // 입력을 잠그기 "전에" 조준 강제 해제 신호를 보낸다.
                        // (잠근 뒤에는 조준 해제 입력이 삼켜져 자세가 고착됨)
                        if (UTheseusEventBusSubsystem* Bus = CachedBus.Get())
                        {
                            Bus->OnForceStopAiming.Broadcast();
                        }

                        ApplyTickerInputLock(true);

                        const float MontageLength = AnimInst->Montage_Play(PrintMontage);
                        if (MontageLength > 0.0f)
                        {
                            FOnMontageEnded EndDelegate;
                            EndDelegate.BindUObject(this, &ATickerPaperManager::HandlePrintMontageEnded);
                            AnimInst->Montage_SetEndDelegate(EndDelegate, PrintMontage);
                        }
                        else
                        {
                            ApplyTickerInputLock(false);
                        }
                    }
                    break;
                }
            }
        }
    }

    // 2. MontagePlayDelay 이후 티켓 스폰
    FTimerDelegate Delegate = FTimerDelegate::CreateLambda([this, Request]()
    {
        FVector SpawnLoc = FVector::ZeroVector;
        FVector SpawnFwd = FVector::ForwardVector;
        FRotator SpawnRot = FRotator::ZeroRotator;
        USceneComponent* SpawnSourceComponent = nullptr;
        FString SocketInfo;

        const bool bUsedNamedScene = TryGetNamedSceneComponentSpawnTransform(Request, SpawnLoc, SpawnFwd, SpawnRot, SpawnSourceComponent, SocketInfo);
        const bool bUsedSocket = !bUsedNamedScene && TryGetSocketSpawnTransform(Request, SpawnLoc, SpawnFwd, SpawnRot, SocketInfo);
        if (!bUsedNamedScene && !bUsedSocket)
        {
            SpawnLoc = ResolveSpawnLocation(Request);
            SpawnFwd = ResolveSpawnForward(Request);
            SpawnRot = SpawnFwd.Rotation();
            SocketInfo = FString::Printf(
                TEXT("Fallback (Component=\"%s\", Socket=\"%s\")"),
                *PreferredSpawnComponentName.ToString(),
                *MuzzleSocketName.ToString());
        }

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        Params.Owner = this;
        Params.Instigator = Request.InstigatorActor ? Request.InstigatorActor->GetInstigator() : nullptr;

        ATickerPaperActor* Spawned = GetWorld()->SpawnActor<ATickerPaperActor>(
            TickerPaperClass,
            SpawnLoc,
            SpawnRot,
            Params);

        if (!Spawned)
        {
            UE_LOG(LogTemp, Warning, TEXT("[TickerPaperManager] Failed to spawn TickerPaperActor."));
            return;
        }

        UMaterialInterface* ResolvedMaterial = nullptr;
        if (TObjectPtr<UMaterialInterface>* Found = PaperMaterialMap.Find(Request.ReasonTag))
        {
            ResolvedMaterial = *Found;
        }

        Spawned->InitializeFromRequest(Request, SpawnLoc, SpawnRot, SpawnSourceComponent, ResolvedMaterial);

        UE_LOG(
            LogTemp,
            Log,
            TEXT("[TickerPaperManager] Spawned paper: Message=\"%s\", Instigator=%s, SpawnSource=%s"),
            *Request.Message.ToString(),
            Request.InstigatorActor ? *Request.InstigatorActor->GetName() : TEXT("None"),
            *SocketInfo);
    });

    GetWorld()->GetTimerManager().SetTimer(MontagePlayTimerHandle, Delegate, MontagePlayDelay, false);
}

void ATickerPaperManager::ApplyTickerInputLock(const bool bLock)
{
    APlayerController* PC = nullptr;
    if (UWorld* World = GetWorld())
    {
        PC = World->GetFirstPlayerController();
    }

    if (bLock)
    {
        if (bTickerInputLocked)
        {
            return;
        }

        if (!IsValid(PC))
        {
            UE_LOG(LogTemp, Warning, TEXT("[TickerPaperManager] Input lock skipped: PlayerController invalid."));
            return;
        }

        PC->SetIgnoreMoveInput(true);
        PC->SetIgnoreLookInput(true);

        if (APawn* Pawn = PC->GetPawn())
        {
            if (Pawn->InputComponent)
            {
                Pawn->DisableInput(PC);
                InputLockedPawn = Pawn;
                bTickerPawnInputDisabled = true;
            }
        }

        InputLockedPlayerController = PC;
        bTickerInputLocked = true;
        UE_LOG(LogTemp, Log, TEXT("[TickerPaperManager] Player input locked for print montage."));
        return;
    }

    if (!bTickerInputLocked)
    {
        return;
    }

    APlayerController* LockedPC = InputLockedPlayerController.Get();
    if (!IsValid(LockedPC))
    {
        LockedPC = PC;
    }

    if (IsValid(LockedPC))
    {
        if (bTickerPawnInputDisabled)
        {
            APawn* LockedPawn = InputLockedPawn.Get();
            if (!IsValid(LockedPawn))
            {
                LockedPawn = LockedPC->GetPawn();
            }

            if (IsValid(LockedPawn) && LockedPawn->GetController() == LockedPC)
            {
                LockedPawn->EnableInput(LockedPC);
            }
        }

        LockedPC->SetIgnoreMoveInput(false);
        LockedPC->SetIgnoreLookInput(false);
        UE_LOG(LogTemp, Log, TEXT("[TickerPaperManager] Player input unlocked after print montage."));
    }

    InputLockedPlayerController.Reset();
    InputLockedPawn.Reset();
    bTickerInputLocked = false;
    bTickerPawnInputDisabled = false;
}

void ATickerPaperManager::HandlePrintMontageEnded(UAnimMontage* Montage, const bool bInterrupted)
{
    if (Montage != PrintMontage)
    {
        return;
    }

    ApplyTickerInputLock(false);
}

bool ATickerPaperManager::TryGetNamedSceneComponentSpawnTransform(
    const FTickerPrintRequest& Request,
    FVector& OutLocation,
    FVector& OutForward,
    FRotator& OutRotation,
    USceneComponent*& OutSourceComponent,
    FString& OutSourceInfo) const
{
    OutSourceComponent = nullptr;

    TArray<AActor*> CandidateActors;

    if (AActor* InstigatorActor = Request.InstigatorActor)
    {
        CandidateActors.Add(InstigatorActor);

        if (AActor* OwnerActor = InstigatorActor->GetOwner())
        {
            CandidateActors.Add(OwnerActor);
        }

        if (APawn* InstigatorPawn = InstigatorActor->GetInstigator())
        {
            CandidateActors.AddUnique(InstigatorPawn);
        }
    }

    for (int32 Index = 0; Index < CandidateActors.Num(); ++Index)
    {
        AActor* Candidate = CandidateActors[Index];
        if (!IsValid(Candidate))
        {
            continue;
        }

        TArray<AActor*> AttachedActors;
        Candidate->GetAttachedActors(AttachedActors, true, true);
        for (AActor* AttachedActor : AttachedActors)
        {
            if (IsValid(AttachedActor))
            {
                CandidateActors.AddUnique(AttachedActor);
            }
        }

        TArray<UChildActorComponent*> ChildActorComponents;
        Candidate->GetComponents<UChildActorComponent>(ChildActorComponents);
        for (UChildActorComponent* ChildActorComponent : ChildActorComponents)
        {
            if (!IsValid(ChildActorComponent))
            {
                continue;
            }

            if (AActor* ChildActor = ChildActorComponent->GetChildActor())
            {
                CandidateActors.AddUnique(ChildActor);
            }
        }
    }

    for (AActor* Candidate : CandidateActors)
    {
        if (!IsValid(Candidate))
        {
            continue;
        }

        TArray<USceneComponent*> SceneComponents;
        Candidate->GetComponents<USceneComponent>(SceneComponents);

        for (USceneComponent* SceneComp : SceneComponents)
        {
            if (!IsValid(SceneComp))
            {
                continue;
            }

            if (SceneComp->GetFName() == PreferredSpawnComponentName)
            {
                OutLocation = SceneComp->GetComponentLocation();
                OutForward = SceneComp->GetForwardVector().GetSafeNormal();
                OutRotation = SceneComp->GetComponentRotation();
                OutSourceComponent = SceneComp;
                OutSourceInfo = FString::Printf(
                    TEXT("SceneComponent %s::%s"),
                    *Candidate->GetName(),
                    *PreferredSpawnComponentName.ToString());
                return true;
            }
        }
    }

    return false;
}

bool ATickerPaperManager::TryGetSocketSpawnTransform(
    const FTickerPrintRequest& Request,
    FVector& OutLocation,
    FVector& OutForward,
    FRotator& OutRotation,
    FString& OutSocketInfo) const
{
    if (AActor* InstigatorActor = Request.InstigatorActor)
    {
        if (USkeletalMeshComponent* SkelMesh = InstigatorActor->FindComponentByClass<USkeletalMeshComponent>())
        {
            if (SkelMesh->DoesSocketExist(MuzzleSocketName))
            {
                OutLocation = SkelMesh->GetSocketLocation(MuzzleSocketName);
                OutRotation = SkelMesh->GetSocketRotation(MuzzleSocketName);
                OutForward = OutRotation.Vector().GetSafeNormal();
                OutSocketInfo = FString::Printf(TEXT("Socket %s:%s"), *SkelMesh->GetName(), *MuzzleSocketName.ToString());
                return true;
            }
        }
    }

    return false;
}

FVector ATickerPaperManager::ResolveSpawnLocation(const FTickerPrintRequest& Request) const
{
    if (AActor* InstigatorActor = Request.InstigatorActor)
    {
        if (USkeletalMeshComponent* SkelMesh = InstigatorActor->FindComponentByClass<USkeletalMeshComponent>())
        {
            if (SkelMesh->DoesSocketExist(MuzzleSocketName))
            {
                return SkelMesh->GetSocketLocation(MuzzleSocketName);
            }
        }

        return InstigatorActor->GetActorLocation();
    }

    return GetActorLocation();
}

FVector ATickerPaperManager::ResolveSpawnForward(const FTickerPrintRequest& Request) const
{
    if (AActor* InstigatorActor = Request.InstigatorActor)
    {
        if (USkeletalMeshComponent* SkelMesh = InstigatorActor->FindComponentByClass<USkeletalMeshComponent>())
        {
            if (SkelMesh->DoesSocketExist(MuzzleSocketName))
            {
                return SkelMesh->GetSocketRotation(MuzzleSocketName).Vector().GetSafeNormal();
            }
        }

        return InstigatorActor->GetActorForwardVector().GetSafeNormal();
    }

    return GetActorForwardVector().GetSafeNormal();
}
