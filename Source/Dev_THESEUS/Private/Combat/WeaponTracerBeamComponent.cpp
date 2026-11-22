// Copyright 2026, Okchun-Yee.

#include "Combat/WeaponTracerBeamComponent.h"

#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

DEFINE_LOG_CATEGORY(LogWeaponTracerBeam);

UWeaponTracerBeamComponent::UWeaponTracerBeamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UWeaponTracerBeamComponent::SpawnTracerBeamFromCamera(AActor* SourceActor, const FVector& StartLocation, float RangeOverride, AActor* InstigatorActor)
{
	FVector Direction = FVector::ZeroVector;
	if (!ResolveDirectionFromSource(SourceActor, Direction))
	{
		UE_LOG(LogWeaponTracerBeam, Warning, TEXT("[TracerBeam] Failed to resolve direction. Source=%s Owner=%s"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(GetOwner()));
		return false;
	}

	const float EffectiveRange = (RangeOverride > 0.0f) ? RangeOverride : DefaultTracerRange;
	return SpawnTracerBeam(StartLocation, Direction, EffectiveRange, InstigatorActor);
}

bool UWeaponTracerBeamComponent::SpawnTracerBeam(const FVector& StartLocation, const FVector& Direction, float Range, AActor* InstigatorActor)
{
	if (!TracerSystem)
	{
		UE_LOG(LogWeaponTracerBeam, Warning, TEXT("[TracerBeam] Spawn skipped: TracerSystem is null. Owner=%s"), *GetNameSafe(GetOwner()));
		return false;
	}

	if (Range <= 0.0f)
	{
		UE_LOG(LogWeaponTracerBeam, Warning, TEXT("[TracerBeam] Spawn skipped: invalid range %.2f. Owner=%s"), Range, *GetNameSafe(GetOwner()));
		return false;
	}

	const FVector SafeDirection = Direction.GetSafeNormal();
	if (SafeDirection.IsNearlyZero())
	{
		UE_LOG(LogWeaponTracerBeam, Warning, TEXT("[TracerBeam] Spawn skipped: invalid direction. Owner=%s"), *GetNameSafe(GetOwner()));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogWeaponTracerBeam, Warning, TEXT("[TracerBeam] Spawn skipped: no world. Owner=%s"), *GetNameSafe(GetOwner()));
		return false;
	}

	const FVector TraceEndCandidate = StartLocation + (SafeDirection * Range);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TracerBeamTrace), false);
	if (bIgnoreOwnerInTrace)
	{
		if (AActor* OwnerActor = GetOwner())
		{
			QueryParams.AddIgnoredActor(OwnerActor);
		}
	}

	if (InstigatorActor)
	{
		QueryParams.AddIgnoredActor(InstigatorActor);
	}

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(HitResult, StartLocation, TraceEndCandidate, TraceChannel, QueryParams);
	const FVector EndLocation = bHit ? HitResult.ImpactPoint : TraceEndCandidate;

	if (bDrawDebugTrace)
	{
		const FColor LineColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugLine(World, StartLocation, EndLocation, LineColor, false, 2.0f, 0, 1.0f);
		DrawDebugSphere(World, EndLocation, 5.0f, 8, LineColor, false, 2.0f);
	}

	UNiagaraComponent* SpawnedComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		TracerSystem,
		StartLocation,
		FRotator::ZeroRotator,
		FVector(1.0f),
		true,
		true,
		ENCPoolMethod::None,
		true);

	if (!SpawnedComp)
	{
		UE_LOG(LogWeaponTracerBeam, Warning, TEXT("[TracerBeam] SpawnSystemAtLocation failed. Owner=%s"), *GetNameSafe(GetOwner()));
		return false;
	}

	SpawnedComp->SetVariableVec3(NiagaraStartParamName, StartLocation);
	SpawnedComp->SetVariableVec3(NiagaraEndParamName, EndLocation);

	UE_LOG(LogWeaponTracerBeam, Verbose, TEXT("[TracerBeam] Spawned: Start=%s End=%s Hit=%s Owner=%s"),
		*StartLocation.ToString(),
		*EndLocation.ToString(),
		bHit ? TEXT("true") : TEXT("false"),
		*GetNameSafe(GetOwner()));

	return true;
}

bool UWeaponTracerBeamComponent::ResolveDirectionFromSource(const AActor* SourceActor, FVector& OutDirection) const
{
	OutDirection = FVector::ZeroVector;

	if (!SourceActor)
	{
		return false;
	}

	if (const UCameraComponent* CameraComp = SourceActor->FindComponentByClass<UCameraComponent>())
	{
		OutDirection = CameraComp->GetForwardVector();
		return !OutDirection.IsNearlyZero();
	}

	if (const APawn* Pawn = Cast<APawn>(SourceActor))
	{
		if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			if (const APlayerCameraManager* CamManager = PC->PlayerCameraManager)
			{
				OutDirection = CamManager->GetCameraRotation().Vector();
				if (!OutDirection.IsNearlyZero())
				{
					return true;
				}
			}
		}
	}

	OutDirection = SourceActor->GetActorForwardVector();
	return !OutDirection.IsNearlyZero();
}
