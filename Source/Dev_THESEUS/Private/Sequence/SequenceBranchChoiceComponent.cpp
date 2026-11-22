// Copyright 2026, Okchun-Yee.

#include "Sequence/SequenceBranchChoiceComponent.h"

#include "AI/AISpawnComponent.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sequence/Sequence27ApproachComponent.h"

USequenceBranchChoiceComponent::USequenceBranchChoiceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	LeftSideData.BranchSide = ETheseusBranchSide::Left;
	RightSideData.BranchSide = ETheseusBranchSide::Right;
}

void USequenceBranchChoiceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoInitializeOnBeginPlay)
	{
		InitializeChoiceState();
	}
}

void USequenceBranchChoiceComponent::InitializeChoiceState()
{
	bChoiceLocked = false;
	SelectedSide = ETheseusBranchSide::None;
	LastSpawnedUnits.Reset();
	CachedPlayerActor = nullptr;

	if (bAutoResolvePlayerOnBeginPlay)
	{
		ResolvePlayerActor(nullptr);
	}

	SetSideWallRemoved(LeftSideData, false);
	SetSideWallRemoved(RightSideData, false);
	SetChoiceTriggerActorsEnabled(true);

	if (LeftSideData.BranchSide != ETheseusBranchSide::Left)
	{
		LogChoiceMessage(FString::Printf(
			TEXT("InitializeChoiceState warning: LeftSideData.BranchSide is %d, expected Left."),
			static_cast<int32>(LeftSideData.BranchSide)),
			true);
	}

	if (RightSideData.BranchSide != ETheseusBranchSide::Right)
	{
		LogChoiceMessage(FString::Printf(
			TEXT("InitializeChoiceState warning: RightSideData.BranchSide is %d, expected Right."),
			static_cast<int32>(RightSideData.BranchSide)),
			true);
	}

	LogChoiceMessage(TEXT("InitializeChoiceState complete."));
}

void USequenceBranchChoiceComponent::ResetChoiceState()
{
	InitializeChoiceState();
}

bool USequenceBranchChoiceComponent::ChooseSide(ETheseusBranchSide InSide, AActor* InstigatorActor)
{
	if (InSide == ETheseusBranchSide::None)
	{
		LogChoiceMessage(TEXT("ChooseSide failed: side is None."), true);
		return false;
	}

	if (bChoiceLocked)
	{
		LogChoiceMessage(FString::Printf(TEXT("ChooseSide ignored: already locked. Selected=%d"), static_cast<int32>(SelectedSide)));
		return false;
	}

	AActor* TargetActor = ResolvePlayerActor(InstigatorActor);
	if (!IsValid(TargetActor))
	{
		LogChoiceMessage(TEXT("ChooseSide failed: player target is invalid."), true);
		return false;
	}

	const FTheseusBranchSideRuntimeData* SelectedData = GetSideData(InSide);
	const FTheseusBranchSideRuntimeData* OppositeData = GetOppositeSideData(InSide);
	if (!SelectedData || !OppositeData)
	{
		LogChoiceMessage(TEXT("ChooseSide failed: side data lookup failed."), true);
		return false;
	}

	bChoiceLocked = true;
	SelectedSide = InSide;
	CachedPlayerActor = TargetActor;

	SetChoiceTriggerActorsEnabled(false);
	SetSideWallRemoved(*SelectedData, true);
	LogChoiceMessage(FString::Printf(
		TEXT("Removed selected-side wall. Selected=%d Wall=%s EncounterSide=%d"),
		static_cast<int32>(InSide),
		*GetNameSafe(SelectedData->TransparentWallActor),
		static_cast<int32>(OppositeData->BranchSide)));

	const bool bEncounterStarted = ActivateEncounterSide(*OppositeData, TargetActor);
	if (!bEncounterStarted)
	{
		LogChoiceMessage(FString::Printf(
			TEXT("ChooseSide rollback: encounter failed to start. Selected=%d Opposite=%d"),
			static_cast<int32>(InSide),
			static_cast<int32>(OppositeData->BranchSide)),
			true);
		InitializeChoiceState();
		return false;
	}

	LogChoiceMessage(FString::Printf(
		TEXT("Choice committed. Selected=%d EncounterStarted=%s SpawnedUnits=%d"),
		static_cast<int32>(SelectedSide),
		bEncounterStarted ? TEXT("true") : TEXT("false"),
		LastSpawnedUnits.Num()));

	return bEncounterStarted;
}

bool USequenceBranchChoiceComponent::ChooseLeft(AActor* InstigatorActor)
{
	return ChooseSide(ETheseusBranchSide::Left, InstigatorActor);
}

bool USequenceBranchChoiceComponent::ChooseRight(AActor* InstigatorActor)
{
	return ChooseSide(ETheseusBranchSide::Right, InstigatorActor);
}

TArray<APawn*> USequenceBranchChoiceComponent::GetLastSpawnedUnits() const
{
	TArray<APawn*> SpawnedUnits;
	SpawnedUnits.Reserve(LastSpawnedUnits.Num());

	for (APawn* SpawnedUnit : LastSpawnedUnits)
	{
		if (IsValid(SpawnedUnit))
		{
			SpawnedUnits.Add(SpawnedUnit);
		}
	}

	return SpawnedUnits;
}

const FTheseusBranchSideRuntimeData* USequenceBranchChoiceComponent::GetSideData(ETheseusBranchSide InSide) const
{
	switch (InSide)
	{
	case ETheseusBranchSide::Left:
		return &LeftSideData;
	case ETheseusBranchSide::Right:
		return &RightSideData;
	default:
		return nullptr;
	}
}

const FTheseusBranchSideRuntimeData* USequenceBranchChoiceComponent::GetOppositeSideData(ETheseusBranchSide InSide) const
{
	switch (InSide)
	{
	case ETheseusBranchSide::Left:
		return &RightSideData;
	case ETheseusBranchSide::Right:
		return &LeftSideData;
	default:
		return nullptr;
	}
}

AActor* USequenceBranchChoiceComponent::ResolvePlayerActor(AActor* InstigatorActor)
{
	if (IsValid(CachedPlayerActor))
	{
		return CachedPlayerActor;
	}

	if (IsValid(PlayerActorOverride))
	{
		CachedPlayerActor = PlayerActorOverride;
		return CachedPlayerActor;
	}

	if (APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
	{
		if (InstigatorPawn->IsPlayerControlled())
		{
			CachedPlayerActor = InstigatorPawn;
			return CachedPlayerActor;
		}
	}

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		CachedPlayerActor = PlayerPawn;
		return CachedPlayerActor;
	}

	return nullptr;
}

void USequenceBranchChoiceComponent::SetChoiceTriggerActorsEnabled(bool bEnabled)
{
	SetManagedActorArrayEnabled(ChoiceTriggerActors, bEnabled, bEnabled);
}

void USequenceBranchChoiceComponent::SetSideWallRemoved(const FTheseusBranchSideRuntimeData& InData, bool bRemoved)
{
	SetManagedActorEnabled(InData.TransparentWallActor, !bRemoved, !bRemoved);
}

bool USequenceBranchChoiceComponent::ActivateEncounterSide(const FTheseusBranchSideRuntimeData& InData, AActor* TargetActor)
{
	LastSpawnedUnits.Reset();

	if (!IsValid(TargetActor))
	{
		LogChoiceMessage(TEXT("ActivateEncounterSide failed: target actor invalid."), true);
		return false;
	}

	UAISpawnComponent* SpawnComponent = nullptr;
	if (IsValid(InData.EncounterSpawnActor))
	{
		SpawnComponent = InData.EncounterSpawnActor->FindComponentByClass<UAISpawnComponent>();
	}

	if (!SpawnComponent)
	{
		LogChoiceMessage(TEXT("ActivateEncounterSide warning: AISpawnComponent not found on EncounterSpawnActor."), true);
	}
	else
	{
		SpawnComponent->Spawn();
		for (APawn* SpawnedUnit : SpawnComponent->GetSpawnedUnits())
		{
			if (IsValid(SpawnedUnit))
			{
				LastSpawnedUnits.Add(SpawnedUnit);
			}
		}
	}

	USequence27ApproachComponent* ApproachComponent = nullptr;
	if (IsValid(InData.Sequence27Actor))
	{
		ApproachComponent = InData.Sequence27Actor->FindComponentByClass<USequence27ApproachComponent>();
	}

	if (!ApproachComponent)
	{
		LogChoiceMessage(TEXT("ActivateEncounterSide warning: Sequence27ApproachComponent not found on Sequence27Actor."), true);
		return LastSpawnedUnits.Num() > 0;
	}

	TArray<APawn*> SpawnedUnitsRaw;
	SpawnedUnitsRaw.Reserve(LastSpawnedUnits.Num());
	for (APawn* SpawnedUnit : LastSpawnedUnits)
	{
		if (IsValid(SpawnedUnit))
		{
			SpawnedUnitsRaw.Add(SpawnedUnit);
		}
	}

	if (SpawnedUnitsRaw.Num() <= 0)
	{
		LogChoiceMessage(TEXT("ActivateEncounterSide failed: no valid spawned units to start Sequence27."), true);
		return false;
	}

	if (bActivateEyesOnEncounterSpawn)
	{
		ApproachComponent->SetEyesActiveForUnits(SpawnedUnitsRaw, true);
		LogChoiceMessage(FString::Printf(TEXT("Activated eyes for spawned units. Count=%d"), SpawnedUnitsRaw.Num()));
	}

	return ApproachComponent->StartApproach(SpawnedUnitsRaw, TargetActor);
}

void USequenceBranchChoiceComponent::SetManagedActorEnabled(AActor* InActor, bool bEnabled, bool bEnableCollision) const
{
	if (!IsValid(InActor))
	{
		return;
	}

	InActor->SetActorHiddenInGame(!bEnabled);
	InActor->SetActorEnableCollision(bEnabled && bEnableCollision);
	InActor->SetActorTickEnabled(bEnabled);

	TInlineComponentArray<UActorComponent*> ActorComponents;
	InActor->GetComponents(ActorComponents);

	for (UActorComponent* ActorComponent : ActorComponents)
	{
		if (!IsValid(ActorComponent))
		{
			continue;
		}

		if (bEnabled)
		{
			if (!ActorComponent->IsActive())
			{
				ActorComponent->Activate(true);
			}
		}
		else
		{
			ActorComponent->Deactivate();
		}

		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(ActorComponent))
		{
			PrimitiveComponent->SetHiddenInGame(!bEnabled, true);
			PrimitiveComponent->SetVisibility(bEnabled, true);
		}
	}
}

void USequenceBranchChoiceComponent::SetManagedActorArrayEnabled(const TArray<TObjectPtr<AActor>>& InActors, bool bEnabled, bool bEnableCollision) const
{
	for (AActor* ManagedActor : InActors)
	{
		SetManagedActorEnabled(ManagedActor, bEnabled, bEnableCollision);
	}
}

void USequenceBranchChoiceComponent::LogChoiceMessage(const FString& Message, bool bWarning) const
{
	if (!bDebugLog && !bWarning)
	{
		return;
	}

	if (bWarning)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BranchChoice] %s"), *Message);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[BranchChoice] %s"), *Message);
}
