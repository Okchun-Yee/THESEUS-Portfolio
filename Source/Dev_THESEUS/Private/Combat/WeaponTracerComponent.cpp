// Copyright 2026, Okchun-Yee.


#include "Combat/WeaponTracerComponent.h"
#include "GameFramework/Actor.h"

// 예광탄(Tracer) 전용 로그 카테고리 정의
DEFINE_LOG_CATEGORY(LogWeaponTracer);


// Sets default values for this component's properties
UWeaponTracerComponent::UWeaponTracerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponTracerComponent::ActivateTracer(int32 Shots)
{
	RemainingTracerShots = FMath::Max(0, Shots);
	bTracerActive = (RemainingTracerShots > 0);

	UE_LOG(LogWeaponTracer, Log, TEXT("[Tracer] ActivateTracer: Shots=%d -> Active=%s Remaining=%d (Owner=%s)"),
		Shots,
		bTracerActive ? TEXT("true") : TEXT("false"),
		RemainingTracerShots,
		*GetNameSafe(GetOwner()));

	BroadcastTracerState();
}

void UWeaponTracerComponent::DeactivateTracer()
{
	bTracerActive = false;
	RemainingTracerShots = 0;

	UE_LOG(LogWeaponTracer, Log, TEXT("[Tracer] DeactivateTracer: Active=false Remaining=0 (Owner=%s)"),
		*GetNameSafe(GetOwner()));

	BroadcastTracerState();
}

bool UWeaponTracerComponent::ShouldUseTracer() const
{
	return bTracerActive && (RemainingTracerShots > 0);
}

void UWeaponTracerComponent::OnShotFired(bool bWasTracer)
{
	// 예광탄 판정으로 실제 발사 성공했을 때만 차감
	if (!bWasTracer || !ShouldUseTracer())
	{
		UE_LOG(LogWeaponTracer, Log, TEXT("[Tracer] OnShotFired: NOT tracer shot -> no consume (Active=%s Remaining=%d Owner=%s)"),
			bTracerActive ? TEXT("true") : TEXT("false"),
			RemainingTracerShots,
			*GetNameSafe(GetOwner()));
		return;

	}
	const int32 Before = RemainingTracerShots;
	RemainingTracerShots = FMath::Max(0, RemainingTracerShots - 1);
	if (RemainingTracerShots == 0)
	{
		bTracerActive = false;
	}

	UE_LOG(LogWeaponTracer, Log, TEXT("[Tracer] OnShotFired: CONSUME tracer (Before=%d After=%d ActiveNow=%s Owner=%s)"),
		Before,
		RemainingTracerShots,
		bTracerActive ? TEXT("true") : TEXT("false"),
		*GetNameSafe(GetOwner()));


	BroadcastTracerState();
}

void UWeaponTracerComponent::BroadcastTracerState()
{
	OnTracerStateChanged.Broadcast(bTracerActive, RemainingTracerShots);
}


