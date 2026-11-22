// Copyright 2026, Okchun-Yee.

#include "Combat/WeaponJamComponent.h"

#include "Sequence/TheseusEventBusSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Components/ChildActorComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogWeaponJam);

UWeaponJamComponent::UWeaponJamComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UWeaponJamComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStartJamCounting)
	{
		StartJamCounting(bResetShotCountOnAutoStart);
	}
}

void UWeaponJamComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(JamSplineEventTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UWeaponJamComponent::StartJamCounting(bool bResetShotCount)
{
	bCountingEnabled = true;

	if (bResetShotCount)
	{
		ShotCount = 0;
	}

	if (bEnableShotCountLog)
	{
		UE_LOG(LogWeaponJam, Log, TEXT("[%s] Start JAM Count. ShotCount=%d"), *GetOwner()->GetName(), ShotCount);
	}
	else
	{
		UE_LOG(LogWeaponJam, Log, TEXT("[%s] Start JAM Count."), *GetOwner()->GetName());
	}
	OnJamCountingChanged.Broadcast(true);
}

void UWeaponJamComponent::StopJamCounting()
{
	bCountingEnabled = false;

	if (bEnableShotCountLog)
	{
		UE_LOG(LogWeaponJam, Log, TEXT("[%s] Stop JAM Count. ShotCount=%d"), *GetOwner()->GetName(), ShotCount);
	}
	else
	{
		UE_LOG(LogWeaponJam, Log, TEXT("[%s] Stop JAM Count."), *GetOwner()->GetName());
	}
	OnJamCountingChanged.Broadcast(false);
}

bool UWeaponJamComponent::CanFire() const
{
	return !bIsJammed;
}

void UWeaponJamComponent::RegisterSuccessfulShot()
{
	if (bIsJammed)
	{
		UE_LOG(LogWeaponJam, Verbose, TEXT("[%s] RegisterSuccessfulShot ignored: already jammed."), *GetOwner()->GetName());
		return;
	}

	if (!bCountingEnabled)
	{
		UE_LOG(LogWeaponJam, Verbose, TEXT("[%s] No Counting"), *GetOwner()->GetName());
		return;
	}

	++ShotCount;

	if (ShotCount >= JamAfterShots)
	{
		SetJammedInternal(true);
	}

	if (bEnableShotCountLog)
	{
		UE_LOG(
			LogWeaponJam,
			Log,
			TEXT("[%s] ShotCount=%d / %d, Jammed=%s"),
			*GetOwner()->GetName(),
			ShotCount,
			JamAfterShots,
			bIsJammed ? TEXT("true") : TEXT("false"));
	}
}

void UWeaponJamComponent::ClearJam(bool bResetShotCount)
{
	SetJammedInternal(false);
	if (bResetShotCount)
	{
		ShotCount = 0;
	}

	if (bEnableShotCountLog)
	{
		UE_LOG(
			LogWeaponJam,
			Log,
			TEXT("[%s] ClearJam: ShotCount=%d, Jammed=%s"),
			*GetOwner()->GetName(),
			ShotCount,
			bIsJammed ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogWeaponJam, Log, TEXT("[%s] ClearJam: Jammed=%s"), *GetOwner()->GetName(), bIsJammed ? TEXT("true") : TEXT("false"));
	}
}

void UWeaponJamComponent::SetJamAfterShots(int32 NewThreshold)
{
	JamAfterShots = FMath::Max(1, NewThreshold);

	UE_LOG(LogWeaponJam, Log, TEXT("[%s] SetJamAfterShots=%d"), *GetOwner()->GetName(), JamAfterShots);
}

void UWeaponJamComponent::ForceJam()
{
	SetJammedInternal(true);
}

void UWeaponJamComponent::SetJammedInternal(bool bNewJammed)
{
	if (bIsJammed == bNewJammed)
	{
		return;
	}

	bIsJammed = bNewJammed;

	if (bEnableShotCountLog)
	{
		UE_LOG(
			LogWeaponJam,
			Log,
			TEXT("[%s] Jam State Change: %s (ShotCount=%d)"),
			*GetOwner()->GetName(),
			bIsJammed ? TEXT("JAMMED") : TEXT("CLEARED"),
			ShotCount);
	}
	else
	{
		UE_LOG(LogWeaponJam, Log, TEXT("[%s] Jam State Change: %s"), *GetOwner()->GetName(), bIsJammed ? TEXT("JAMMED") : TEXT("CLEARED"));
	}

	OnJamStateChanged.Broadcast(bIsJammed, ShotCount);

	if (bIsJammed)
	{
		// Spline 이벤트를 쓰지 않는 경우에도 AI JAM 시점에 플레이어 JAM 전파를 허용한다.
		if (bPropagateJamToPlayerOnSplineEvent && !bEnableJamSplineEvent)
		{
			PropagateJamToPlayerWeapon();
		}

		if (bEnableJamSplineEvent)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(JamSplineEventTimerHandle);
				World->GetTimerManager().SetTimer(
					JamSplineEventTimerHandle,
					this,
					&UWeaponJamComponent::BroadcastJamSplineEvent,
					FMath::Max(0.0f, JamSplineEventDelay),
					false);

				UE_LOG(
					LogWeaponJam,
					Log,
					TEXT("[%s] Scheduled JamSplineEvent in %.2f sec."),
					*GetOwner()->GetName(),
					JamSplineEventDelay);
			}
		}

			if (bEnableTickerPrintOnJam)
			{
				if (UWorld* World = GetWorld())
				{
					if (UGameInstance* GI = World->GetGameInstance())
					{
						if (UTheseusEventBusSubsystem* Bus = GI->GetSubsystem<UTheseusEventBusSubsystem>())
						{
							FTickerPrintRequest Request;
							Request.Message = FText::FromString(TEXT("WEAPON JAMMED"));
							Request.ReasonTag = FGameplayTag::RequestGameplayTag(TEXT("Theseus.Ticker.Jam"));
							Request.EjectDistance = TickerEjectDistance;
							Request.LifeTime = 0.0f;
							Request.InstigatorActor = GetOwner();

							if (bEnableShotCountLog)
							{
								UE_LOG(
									LogWeaponJam,
									Log,
									TEXT("[%s] Publishing TickerPrintRequest (Message=\"%s\", ShotCount=%d)"),
									*GetOwner()->GetName(),
									*Request.Message.ToString(),
									ShotCount);
							}
							else
							{
								UE_LOG(
									LogWeaponJam,
									Log,
									TEXT("[%s] Publishing TickerPrintRequest (Message=\"%s\")"),
									*GetOwner()->GetName(),
									*Request.Message.ToString());
							}

							Bus->PublishTickerPrintRequest(Request);
						}
					}
				}
			}

	}
	else
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(JamSplineEventTimerHandle);
		}
	}
}

void UWeaponJamComponent::BroadcastJamSplineEvent()
{
	if (!bIsJammed)
	{
		return;
	}

	if (bEnableShotCountLog)
	{
		UE_LOG(
			LogWeaponJam,
			Log,
			TEXT("[%s] Broadcast JamSplineEvent (ShotCount=%d)"),
			*GetOwner()->GetName(),
			ShotCount);
	}
	else
	{
		UE_LOG(LogWeaponJam, Log, TEXT("[%s] Broadcast JamSplineEvent"), *GetOwner()->GetName());
	}

	OnJamSplineEventRequested.Broadcast(ShotCount);

	if (bPropagateJamToPlayerOnSplineEvent)
	{
		PropagateJamToPlayerWeapon();
	}
}

void UWeaponJamComponent::PropagateJamToPlayerWeapon()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(PlayerPawn))
	{
		UE_LOG(LogWeaponJam, Warning, TEXT("[%s] PropagateJamToPlayerWeapon failed: PlayerPawn invalid."), *GetNameSafe(GetOwner()));
		return;
	}

	if (PlayerPawn == GetOwner())
	{
		return;
	}

	// Fast path: reuse previously resolved player jam component when still valid.
	if (CachedPlayerPawn.Get() == PlayerPawn && CachedPlayerWeaponJam.IsValid() && CachedPlayerWeaponJam.Get() != this)
	{
		UWeaponJamComponent* CachedJam = CachedPlayerWeaponJam.Get();
		if (!CachedJam->IsJammed())
		{
			CachedJam->ForceJam();
		}

		UE_LOG(
			LogWeaponJam,
			Log,
			TEXT("[%s] PropagateJamToPlayerWeapon (Cached): Player=%s TargetActor=%s Jammed=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(PlayerPawn),
			*GetNameSafe(CachedJam->GetOwner()),
			CachedJam->IsJammed() ? TEXT("true") : TEXT("false"));
		return;
	}

	auto TryForceJamOnActor = [this, PlayerPawn](AActor* CandidateActor, const TCHAR* ContextLabel) -> bool
	{
		if (!IsValid(CandidateActor))
		{
			return false;
		}

		if (UWeaponJamComponent* CandidateJam = CandidateActor->FindComponentByClass<UWeaponJamComponent>())
		{
			if (CandidateJam == this)
			{
				return false;
			}

			if (!CandidateJam->IsJammed())
			{
				CandidateJam->ForceJam();
			}

			CachedPlayerPawn = PlayerPawn;
			CachedPlayerWeaponJam = CandidateJam;

			UE_LOG(
				LogWeaponJam,
				Log,
				TEXT("[%s] PropagateJamToPlayerWeapon (%s): Player=%s TargetActor=%s Jammed=%s"),
				*GetNameSafe(GetOwner()),
				ContextLabel,
				*GetNameSafe(PlayerPawn),
				*GetNameSafe(CandidateActor),
				CandidateJam->IsJammed() ? TEXT("true") : TEXT("false"));

			return true;
		}

		return false;
	};

	// 1) Pawn 자신에 WeaponJamComponent가 있는 경우 우선 사용.
	if (TryForceJamOnActor(PlayerPawn, TEXT("PlayerPawn")))
	{
		return;
	}

	// 2) Pawn 부착 Actor(무기 Actor 등)에서 WeaponJamComponent 탐색.
	TArray<AActor*> AttachedActors;
	PlayerPawn->GetAttachedActors(AttachedActors, true, true);
	for (AActor* AttachedActor : AttachedActors)
	{
		if (TryForceJamOnActor(AttachedActor, TEXT("AttachedActor")))
		{
			return;
		}
	}

	// 3) Pawn의 ChildActorComponent 소유 Actor(무기 ChildActor 포함) 탐색.
	TArray<UChildActorComponent*> ChildActorComponents;
	PlayerPawn->GetComponents<UChildActorComponent>(ChildActorComponents);
	for (UChildActorComponent* ChildComp : ChildActorComponents)
	{
		if (!IsValid(ChildComp))
		{
			continue;
		}

		if (TryForceJamOnActor(ChildComp->GetChildActor(), TEXT("ChildActorComponent")))
		{
			return;
		}
	}

	UE_LOG(
		LogWeaponJam,
		Warning,
		TEXT("[%s] PropagateJamToPlayerWeapon failed: no UWeaponJamComponent found on Player=%s, attached actors, or child actors"),
		*GetNameSafe(GetOwner()),
		*PlayerPawn->GetName());
}
