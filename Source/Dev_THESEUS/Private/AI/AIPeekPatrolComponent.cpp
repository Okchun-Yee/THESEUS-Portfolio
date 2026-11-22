// Copyright 2026, Okchun-Yee.

#include "AI/AIPeekPatrolComponent.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

UAIPeekPatrolComponent::UAIPeekPatrolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAIPeekPatrolComponent::ConfigurePeekPatrol(AActor* InLeftPoint, AActor* InRightPoint, float InWaitTime, float InTimeoutToRush)
{
	PeekLeftPoint = InLeftPoint;
	PeekRightPoint = InRightPoint;

	if (InWaitTime >= 0.0f)
	{
		PeekWaitTime = InWaitTime;
	}

	if (InTimeoutToRush >= 0.0f)
	{
		PeekTimeoutToRush = InTimeoutToRush;
	}
}

bool UAIPeekPatrolComponent::StartPeekPatrol()
{
	if (!IsValid(PeekLeftPoint) || !IsValid(PeekRightPoint))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AIPeekPatrol] Start failed: left/right point invalid. Owner=%s"), *GetNameSafe(GetOwner()));
		return false;
	}

	AAIController* AIController = ResolveAIController();
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AIPeekPatrol] Start failed: no AIController. Owner=%s"), *GetNameSafe(GetOwner()));
		return false;
	}

	// Mark peek mode active. BT should use this key to block only chase branch.
	if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
	{
		BlackboardComp->SetValueAsBool(TEXT("IsPeekPatrolActive"), true);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PeekUpdateTimerHandle);
	}

	bPeekActive = true;
	ElapsedTotal = 0.0f;
	ElapsedInState = 0.0f;
	EnterState(EAIPeekPatrolState::MoveLeft);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PeekUpdateTimerHandle,
			this,
			&UAIPeekPatrolComponent::UpdatePeekPatrol,
			FMath::Max(0.05f, UpdateInterval),
			true);
	}

	if (bEnablePeekLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[AIPeekPatrol] Peek Start: Owner=%s Left=%s Right=%s Wait=%.2f Timeout=%.2f"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(PeekLeftPoint),
			*GetNameSafe(PeekRightPoint),
			PeekWaitTime,
			PeekTimeoutToRush);
	}

	return true;
}

void UAIPeekPatrolComponent::StopPeekPatrol(bool bForceRush)
{
	if (bForceRush)
	{
		EnterRush(TEXT("ForceStop"));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PeekUpdateTimerHandle);
	}

	if (AAIController* AIController = ResolveAIController())
	{
		AIController->StopMovement();
		if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
		{
			BlackboardComp->SetValueAsBool(TEXT("IsPeekPatrolActive"), false);
		}
	}

	bPeekActive = false;
	EnterState(EAIPeekPatrolState::None);
}

void UAIPeekPatrolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPeekPatrol(false);
	Super::EndPlay(EndPlayReason);
}

void UAIPeekPatrolComponent::UpdatePeekPatrol()
{
	if (!bPeekActive)
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		StopPeekPatrol(false);
		return;
	}

	const float DeltaTime = FMath::Max(0.05f, UpdateInterval);
	ElapsedTotal += DeltaTime;
	ElapsedInState += DeltaTime;

	if (PeekTimeoutToRush > 0.0f && ElapsedTotal >= PeekTimeoutToRush)
	{
		EnterRush(TEXT("Timeout"));
		return;
	}

	switch (CurrentState)
	{
	case EAIPeekPatrolState::MoveLeft:
	{
		if (!IsValid(PeekLeftPoint))
		{
			EnterRush(TEXT("InvalidLeftPoint"));
			return;
		}

		if (MoveToPoint(PeekLeftPoint))
		{
			EnterState(EAIPeekPatrolState::WaitLeft);
		}
		break;
	}
	case EAIPeekPatrolState::WaitLeft:
	{
		if (ElapsedInState >= FMath::Max(0.0f, PeekWaitTime))
		{
			EnterState(EAIPeekPatrolState::MoveRight);
		}
		break;
	}
	case EAIPeekPatrolState::MoveRight:
	{
		if (!IsValid(PeekRightPoint))
		{
			EnterRush(TEXT("InvalidRightPoint"));
			return;
		}

		if (MoveToPoint(PeekRightPoint))
		{
			EnterState(EAIPeekPatrolState::WaitRight);
		}
		break;
	}
	case EAIPeekPatrolState::WaitRight:
	{
		if (ElapsedInState >= FMath::Max(0.0f, PeekWaitTime))
		{
			EnterState(EAIPeekPatrolState::MoveLeft);
		}
		break;
	}
	default:
		break;
	}
}

bool UAIPeekPatrolComponent::MoveToPoint(AActor* TargetPoint)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !IsValid(TargetPoint))
	{
		return false;
	}

	const FVector CurrentLocation = OwnerPawn->GetActorLocation();
	const FVector TargetLocation = TargetPoint->GetActorLocation();
	const float Distance = FVector::Dist2D(CurrentLocation, TargetLocation);
	if (Distance <= MoveAcceptanceRadius)
	{
		if (AAIController* AIController = ResolveAIController())
		{
			AIController->StopMovement();
		}
		return true;
	}

	if (AAIController* AIController = ResolveAIController())
	{
		const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(
			TargetPoint,
			MoveAcceptanceRadius,
			true,
			true,
			true,
			nullptr,
			true);

		if (bEnablePeekLog)
		{
			const TCHAR* ResultText = TEXT("Unknown");
			switch (MoveResult)
			{
			case EPathFollowingRequestResult::Failed:
				ResultText = TEXT("Failed");
				break;
			case EPathFollowingRequestResult::AlreadyAtGoal:
				ResultText = TEXT("AlreadyAtGoal");
				break;
			case EPathFollowingRequestResult::RequestSuccessful:
				ResultText = TEXT("RequestSuccessful");
				break;
			default:
				break;
			}

			UE_LOG(LogTemp, Log, TEXT("[AIPeekPatrol] MoveLeft/Right result: Owner=%s Target=%s Dist=%.1f Result=%s"),
				*OwnerPawn->GetName(),
				*TargetPoint->GetName(),
				Distance,
				ResultText);
		}

		if (MoveResult == EPathFollowingRequestResult::Failed)
		{
			EnterRush(TEXT("MoveFailed"));
		}
		else if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			AIController->StopMovement();
			return true;
		}
	}
	else
	{
		EnterRush(TEXT("NoController"));
	}

	return false;
}

void UAIPeekPatrolComponent::EnterState(EAIPeekPatrolState NewState)
{
	CurrentState = NewState;
	ElapsedInState = 0.0f;
}

void UAIPeekPatrolComponent::EnterRush(const TCHAR* Reason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PeekUpdateTimerHandle);
	}

	bPeekActive = false;
	EnterState(EAIPeekPatrolState::Rush);

	AAIController* AIController = ResolveAIController();
	if (AIController)
	{
		AIController->StopMovement();
		if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
		{
			BlackboardComp->SetValueAsBool(TEXT("IsPeekPatrolActive"), false);
		}
		ApplyRushBlackboardTarget(AIController);
	}

	if (bEnablePeekLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AIPeekPatrol] Timeout -> Rush: Owner=%s Reason=%s Elapsed=%.2f"),
			*GetNameSafe(GetOwner()),
			Reason ? Reason : TEXT("Unknown"),
			ElapsedTotal);
	}

	OnPeekRush.Broadcast();
}

AAIController* UAIPeekPatrolComponent::ResolveAIController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? Cast<AAIController>(OwnerPawn->GetController()) : nullptr;
}

void UAIPeekPatrolComponent::ApplyRushBlackboardTarget(AAIController* AIController) const
{
	if (!AIController)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return;
	}

	APawn* ChaseTarget = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!ChaseTarget)
	{
		ChaseTarget = AIController->GetPawn();
	}

	BlackboardComp->SetValueAsObject(TEXT("Chase_Target"), ChaseTarget);
}
