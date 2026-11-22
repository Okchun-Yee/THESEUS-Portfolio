// Copyright 2026, Okchun-Yee.

#include "Sequence/Sequence27ApproachComponent.h"
#include "AI/AISpawnComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

USequence27ApproachComponent::USequence27ApproachComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool USequence27ApproachComponent::StartApproach(const TArray<APawn*>& InSpawnedUnits, AActor* TargetActor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (!IsValid(TargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sequence27Approach] StartApproach failed: TargetActor invalid."));
		return false;
	}

	ManagedUnits.Reset();
	LastDistanceByUnit.Reset();
	StuckTicksByUnit.Reset();
	for (APawn* Unit : InSpawnedUnits)
	{
		if (IsValid(Unit))
		{
			ManagedUnits.Add(Unit);
		}
	}

	if (ManagedUnits.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sequence27Approach] StartApproach failed: no valid units."));
		return false;
	}

	StopApproachInternal(false);

	ObserveTarget = TargetActor;
	bApproachActive = true;
	ElapsedApproachTime = 0.0f;
	LastInRangeUnits = 0;
	LastValidUnits = ManagedUnits.Num();

	// Recover unit movement mode if it was left disabled by previous sequence states.
	for (APawn* Unit : ManagedUnits)
	{
		if (ACharacter* CharacterUnit = Cast<ACharacter>(Unit))
		{
			if (UCharacterMovementComponent* MoveComp = CharacterUnit->GetCharacterMovement())
			{
				if (MoveComp->MovementMode == MOVE_None)
				{
					MoveComp->SetMovementMode(MOVE_Walking);
					if (bEnableApproachLog)
					{
						UE_LOG(LogTemp, Warning, TEXT("[Sequence27Approach] Restored movement mode: Unit=%s -> Walking"), *Unit->GetName());
					}
				}
			}
		}
	}

	World->GetTimerManager().SetTimer(
		ApproachTimerHandle,
		this,
		&USequence27ApproachComponent::UpdateApproach,
		FMath::Max(UpdateInterval, 0.05f),
		true);

	if (bEnableApproachLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[Sequence27Approach] StartApproach: Units=%d Target=%s TriggerDist=%.1f"),
			ManagedUnits.Num(),
			*GetNameSafe(TargetActor),
			EncircleTriggerDistance);
	}

	return true;
}

void USequence27ApproachComponent::SetEyesActiveForUnits(const TArray<APawn*>& InUnits, bool bActive)
{
	for (APawn* Unit : InUnits)
	{
		TrySetEyesActive(Unit, bActive);
	}
}

void USequence27ApproachComponent::EnterObserveModeForUnits(const TArray<APawn*>& InUnits, AActor* TargetActor, bool bEnableEyes)
{
	ObserveTarget = TargetActor;

	for (APawn* Unit : InUnits)
	{
		if (!IsValid(Unit))
		{
			continue;
		}

		if (bEnableEyes)
		{
			TrySetEyesActive(Unit, true);
		}

		EnterObserveModeForUnit(Unit);
	}
}

void USequence27ApproachComponent::SetEyesActiveForSpawnedUnits(bool bActive)
{
	const UAISpawnComponent* SpawnComponent = GetOwner() ? GetOwner()->FindComponentByClass<UAISpawnComponent>() : nullptr;
	if (!SpawnComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sequence27Approach] SetEyesActiveForSpawnedUnits failed: AISpawnComponent not found on owner."));
		return;
	}

	SetEyesActiveForUnits(SpawnComponent->GetSpawnedUnits(), bActive);
}

void USequence27ApproachComponent::EnterObserveModeForSpawnedUnits(AActor* TargetActor, bool bEnableEyes)
{
	const UAISpawnComponent* SpawnComponent = GetOwner() ? GetOwner()->FindComponentByClass<UAISpawnComponent>() : nullptr;
	if (!SpawnComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sequence27Approach] EnterObserveModeForSpawnedUnits failed: AISpawnComponent not found on owner."));
		return;
	}

	EnterObserveModeForUnits(SpawnComponent->GetSpawnedUnits(), TargetActor, bEnableEyes);
}

void USequence27ApproachComponent::StopApproach()
{
	StopApproachInternal(false);
}

void USequence27ApproachComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopApproachInternal(false);
	Super::EndPlay(EndPlayReason);
}

void USequence27ApproachComponent::UpdateApproach()
{
	if (!bApproachActive)
	{
		return;
	}

	AActor* Target = ObserveTarget;
	if (!IsValid(Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sequence27Approach] UpdateApproach stopped: Target invalid."));
		StopApproachInternal(false);
		return;
	}

	int32 ValidUnits = 0;
	int32 InRangeUnits = 0;
	const FVector TargetLocation = Target->GetActorLocation();

	for (int32 i = ManagedUnits.Num() - 1; i >= 0; --i)
	{
		APawn* Unit = ManagedUnits[i];
		if (!IsValid(Unit))
		{
			ManagedUnits.RemoveAtSwap(i);
			continue;
		}

		++ValidUnits;

		const float DistToTarget = FVector::Dist(Unit->GetActorLocation(), TargetLocation);
		const bool bInRange = DistToTarget <= EncircleTriggerDistance;
		const TWeakObjectPtr<APawn> UnitKey(Unit);

		// Detect "request succeeds but unit doesn't move" and trigger fallback.
		int32& StuckTicks = StuckTicksByUnit.FindOrAdd(UnitKey);
		if (const float* PrevDist = LastDistanceByUnit.Find(UnitKey))
		{
			const float Delta = *PrevDist - DistToTarget;
			if (Delta < MinProgressDistancePerTick)
			{
				++StuckTicks;
			}
			else
			{
				StuckTicks = 0;
			}
		}
		LastDistanceByUnit.Add(UnitKey, DistToTarget);

		if (bInRange)
		{
			++InRangeUnits;
			StuckTicks = 0;
		}

		if (AAIController* AIController = Cast<AAIController>(Unit->GetController()))
		{
			ApplyLookAtTarget(Unit, Target);

			if (bInRange)
			{
				StopUnitMovement(Unit, false);
				if (bEnableApproachLog)
				{
					UE_LOG(LogTemp, Log, TEXT("[Sequence27Approach] InRange stop: Unit=%s Dist=%.1f"),
						*Unit->GetName(),
						DistToTarget);
				}
			}
			else
			{
				EPathFollowingRequestResult::Type MoveResult = EPathFollowingRequestResult::AlreadyAtGoal;
				bool bIssuedMoveRequest = true;

				if (bOnlyRequestMoveWhenNeeded &&
					AIController->GetMoveStatus() == EPathFollowingStatus::Moving &&
					StuckTicks < StuckTickThreshold)
				{
					bIssuedMoveRequest = false;
					if (bEnableApproachLog)
					{
						UE_LOG(LogTemp, Log, TEXT("[Sequence27Approach] MoveTo skipped (already moving): Unit=%s Dist=%.1f"),
							*Unit->GetName(),
							DistToTarget);
					}
				}

				if (bIssuedMoveRequest)
				{
					MoveResult = AIController->MoveToActor(Target, ApproachAcceptanceRadius, true, true, true, nullptr, true);
				}

				if (bEnableApproachLog && bIssuedMoveRequest)
				{
					const TCHAR* MoveResultText = TEXT("Unknown");
					switch (MoveResult)
					{
					case EPathFollowingRequestResult::Failed:
						MoveResultText = TEXT("Failed");
						break;
					case EPathFollowingRequestResult::AlreadyAtGoal:
						MoveResultText = TEXT("AlreadyAtGoal");
						break;
					case EPathFollowingRequestResult::RequestSuccessful:
						MoveResultText = TEXT("RequestSuccessful");
						break;
					default:
						break;
					}

					UE_LOG(LogTemp, Log, TEXT("[Sequence27Approach] MoveTo: Unit=%s Dist=%.1f Result=%s"),
						*Unit->GetName(),
						DistToTarget,
						MoveResultText);
				}

				const bool bMoveRequestFailed = bIssuedMoveRequest && (MoveResult == EPathFollowingRequestResult::Failed);

				if (bMoveRequestFailed && bUseDirectMoveFallbackOnMoveFail)
				{
					TryDirectMoveToTarget(Unit, TargetLocation, FMath::Max(UpdateInterval, 0.05f));
				}
				else if (bUseDirectMoveFallbackOnMoveFail && StuckTicks >= StuckTickThreshold)
				{
					if (bEnableApproachLog)
					{
						UE_LOG(LogTemp, Warning, TEXT("[Sequence27Approach] StuckDetected: Unit=%s StuckTicks=%d Dist=%.1f -> DirectMoveFallback"),
							*Unit->GetName(),
							StuckTicks,
							DistToTarget);
					}
					TryDirectMoveToTarget(Unit, TargetLocation, FMath::Max(UpdateInterval, 0.05f));
					StuckTicks = 0;
				}
			}
		}
		else if (bEnableApproachLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Sequence27Approach] No AIController: Unit=%s Controller=%s"),
				*Unit->GetName(),
				*GetNameSafe(Unit->GetController()));
		}
	}

	LastValidUnits = ValidUnits;
	LastInRangeUnits = InRangeUnits;
	ElapsedApproachTime += FMath::Max(UpdateInterval, 0.05f);

	if (bEnableApproachLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[Sequence27Approach] Tick: InRange=%d Valid=%d Time=%.2f"),
			InRangeUnits,
			ValidUnits,
			ElapsedApproachTime);
	}

	if (ShouldCompleteApproach(InRangeUnits, ValidUnits))
	{
		if (bEnableApproachLog)
		{
			UE_LOG(LogTemp, Log, TEXT("[Sequence27Approach] Complete: InRange=%d Valid=%d"), InRangeUnits, ValidUnits);
		}
		StopApproachInternal(true);
		return;
	}

	if (MaxApproachDuration > 0.0f && ElapsedApproachTime >= MaxApproachDuration)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sequence27Approach] Timeout reached (%.2fs). Forcing completion."), ElapsedApproachTime);
		StopApproachInternal(true);
	}
}

void USequence27ApproachComponent::StopApproachInternal(bool bReachedCondition)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ApproachTimerHandle);
	}

	const bool bWasActive = bApproachActive;
	bApproachActive = false;
	LastDistanceByUnit.Reset();
	StuckTicksByUnit.Reset();

	if (bWasActive)
	{
		for (APawn* Unit : ManagedUnits)
		{
			if (!IsValid(Unit))
			{
				continue;
			}

			if (bReachedCondition)
			{
				EnterObserveModeForUnit(Unit);
			}
			else
			{
				StopUnitMovement(Unit, false);
			}
		}
	}

	if (bWasActive && bReachedCondition)
	{
		OnApproachCompleted.Broadcast(LastInRangeUnits, LastValidUnits);
	}
}

bool USequence27ApproachComponent::ShouldCompleteApproach(int32 InRangeUnits, int32 ValidUnits) const
{
	if (ValidUnits <= 0)
	{
		return false;
	}

	if (bRequireAllUnitsInRange)
	{
		return InRangeUnits >= ValidUnits;
	}

	return InRangeUnits >= FMath::Max(1, MinUnitsInRange);
}

void USequence27ApproachComponent::StopUnitMovement(APawn* Unit, bool bDisableMovement) const
{
	if (!IsValid(Unit))
	{
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(Unit->GetController()))
	{
		AIController->StopMovement();
	}

	if (ACharacter* CharacterUnit = Cast<ACharacter>(Unit))
	{
		if (UCharacterMovementComponent* MoveComp = CharacterUnit->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
			if (bDisableMovement)
			{
				MoveComp->DisableMovement();
			}
		}
	}
}

void USequence27ApproachComponent::EnterObserveModeForUnit(APawn* Unit) const
{
	if (!IsValid(Unit))
	{
		return;
	}

	StopUnitMovement(Unit, bDisableMovementOnCompletion);

	if (AAIController* AIController = Cast<AAIController>(Unit->GetController()))
	{
		if (bStopBrainLogicOnCompletion)
		{
			if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
			{
				BrainComponent->StopLogic(BrainStopReason);
			}
		}
	}

	if (IsValid(ObserveTarget))
	{
		ApplyLookAtTarget(Unit, ObserveTarget);
	}

	if (bEnableApproachLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[Sequence27Approach] EnterObserveMode: Unit=%s StopBrain=%s DisableMovement=%s"),
			*Unit->GetName(),
			bStopBrainLogicOnCompletion ? TEXT("true") : TEXT("false"),
			bDisableMovementOnCompletion ? TEXT("true") : TEXT("false"));
	}
}

bool USequence27ApproachComponent::TrySetEyesActive(APawn* Unit, bool bActive) const
{
	if (!IsValid(Unit))
	{
		return false;
	}

	static const FName SetEyesActiveName(TEXT("SetEyesActive"));
	UFunction* SetEyesActiveFunction = Unit->FindFunction(SetEyesActiveName);
	if (!SetEyesActiveFunction)
	{
		if (bEnableApproachLog)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[Sequence27Approach] SetEyesActive not found on %s"), *GetNameSafe(Unit));
		}
		return false;
	}

	struct FSetEyesActiveParams
	{
		bool bIsActive = false;
	};

	FSetEyesActiveParams Params;
	Params.bIsActive = bActive;
	Unit->ProcessEvent(SetEyesActiveFunction, &Params);

	if (bEnableApproachLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[Sequence27Approach] SetEyesActive: Unit=%s Active=%s"),
			*GetNameSafe(Unit),
			bActive ? TEXT("true") : TEXT("false"));
	}

	return true;
}

void USequence27ApproachComponent::TryDirectMoveToTarget(APawn* Unit, const FVector& TargetLocation, float DeltaTime) const
{
	if (!IsValid(Unit) || DirectMoveSpeed <= 0.0f || DeltaTime <= 0.0f)
	{
		return;
	}

	FVector Current = Unit->GetActorLocation();
	FVector ToTarget = TargetLocation - Current;
	ToTarget.Z = 0.0f;

	const float Dist2D = ToTarget.Size();
	if (Dist2D <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector Dir = ToTarget / Dist2D;
	const float MaxStep = DirectMoveSpeed * DeltaTime;
	const float Step = FMath::Min(MaxStep, Dist2D);

	// Prefer CharacterMovement path so locomotion animation gets velocity updates.
	if (bUseInputDrivenFallback)
	{
		if (ACharacter* CharacterUnit = Cast<ACharacter>(Unit))
		{
			if (UCharacterMovementComponent* MoveComp = CharacterUnit->GetCharacterMovement())
			{
				if (MoveComp->MovementMode == MOVE_None)
				{
					MoveComp->SetMovementMode(MOVE_Walking);
				}

				const FVector MoveDir2D = FVector(Dir.X, Dir.Y, 0.0f).GetSafeNormal();
				CharacterUnit->AddMovementInput(MoveDir2D, 1.0f, false);
				MoveComp->Velocity = MoveDir2D * DirectMoveSpeed;
				CharacterUnit->SetActorRotation(FRotator(0.0f, MoveDir2D.Rotation().Yaw, 0.0f));

				if (bEnableApproachLog)
				{
					UE_LOG(LogTemp, Log, TEXT("[Sequence27Approach] DirectMoveInputFallback: Unit=%s DistBefore=%.1f"),
						*Unit->GetName(),
						Dist2D);
				}
				return;
			}
		}
	}

	const FVector NewLocation = Current + (Dir * Step);
	Unit->SetActorLocationAndRotation(NewLocation, FRotator(0.0f, Dir.Rotation().Yaw, 0.0f), true);

	if (bEnableApproachLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[Sequence27Approach] DirectMoveFallback: Unit=%s Step=%.1f DistBefore=%.1f"),
			*Unit->GetName(),
			Step,
			Dist2D);
	}
}

void USequence27ApproachComponent::ApplyLookAtTarget(APawn* Unit, AActor* Target) const
{
	if (!IsValid(Unit) || !IsValid(Target))
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(Unit->GetController());
	if (!AIController)
	{
		return;
	}

	AIController->SetFocus(Target);

	FVector ToTarget = Target->GetActorLocation() - Unit->GetActorLocation();
	ToTarget.Z = 0.0f;
	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator LookYaw = FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f);
	AIController->SetControlRotation(LookYaw);
	Unit->SetActorRotation(LookYaw);

	if (ACharacter* CharacterUnit = Cast<ACharacter>(Unit))
	{
		CharacterUnit->bUseControllerRotationYaw = true;
		if (UCharacterMovementComponent* MoveComp = CharacterUnit->GetCharacterMovement())
		{
			MoveComp->bOrientRotationToMovement = false;
		}
	}
}
