// Copyright 2026, Okchun-Yee.

#include "Common/Trigger/BreakableLightComponent.h"

#include "Common/Trigger/LightFlickerModeComponent.h"
#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

UBreakableLightComponent::UBreakableLightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBreakableLightComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveTargetsIfNeeded();

	if (TargetLight)
	{
		CachedOriginalIntensity = TargetLight->Intensity;
		bCachedOriginalVisibility = TargetLight->IsVisible();
	}

	if (ResolvedIntactMesh)
	{
		bCachedIntactMeshVisibility = ResolvedIntactMesh->IsVisible();
	}

	if (ResolvedBrokenMesh)
	{
		bCachedBrokenMeshVisibility = ResolvedBrokenMesh->IsVisible();

		if (const UPrimitiveComponent* BrokenMeshPrimitive = Cast<UPrimitiveComponent>(ResolvedBrokenMesh))
		{
			bCachedBrokenMeshSimulatePhysics = BrokenMeshPrimitive->IsSimulatingPhysics();
			bCachedBrokenMeshEnableGravity = BrokenMeshPrimitive->IsGravityEnabled();
		}
	}

}

void UBreakableLightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BreakSequenceTimerHandle);
		World->GetTimerManager().ClearTimer(BrokenMeshPhysicsTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

bool UBreakableLightComponent::StartBreakSequence()
{
	if (bIsBroken || bBreakInProgress)
	{
		if (bEnableDebugLog)
		{
			UE_LOG(LogTemp, Log, TEXT("[BreakableLight] StartBreakSequence skipped. Owner=%s Broken=%s InProgress=%s"),
				*GetNameSafe(GetOwner()),
				bIsBroken ? TEXT("true") : TEXT("false"),
				bBreakInProgress ? TEXT("true") : TEXT("false"));
		}
		return false;
	}

	ResolveTargetsIfNeeded();

	bBreakInProgress = true;
	OnBreakSequenceStarted.Broadcast();

	const bool bCanUsePreBreak = bUsePreBreakFlicker && FlickerComponent && PreBreakDelaySeconds > 0.0f;
	if (bCanUsePreBreak)
	{
		FlickerComponent->SetStartCallsBlocked(false);
		FlickerComponent->StopAndRestore();
		FlickerComponent->StartFlickerBurst(PreBreakBlinkCount, PreBreakIntervalSeconds);

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				BreakSequenceTimerHandle,
				this,
				&UBreakableLightComponent::BreakLightFinal,
				PreBreakDelaySeconds,
				false);
		}

		return true;
	}

	BreakLightFinal();
	return true;
}

void UBreakableLightComponent::BreakLightFinal()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BreakSequenceTimerHandle);
		World->GetTimerManager().ClearTimer(BrokenMeshPhysicsTimerHandle);
	}

	if (bIsBroken)
	{
		bBreakInProgress = false;
		return;
	}

	ResolveTargetsIfNeeded();
	ApplyFinalBrokenState();

	bIsBroken = true;
	bBreakInProgress = false;

	OnLightBroken.Broadcast();

	if (bEnableDebugLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[BreakableLight] Light broken. Owner=%s"), *GetNameSafe(GetOwner()));
	}
}

void UBreakableLightComponent::ResetBreakState(bool bRestoreLightState)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BreakSequenceTimerHandle);
		World->GetTimerManager().ClearTimer(BrokenMeshPhysicsTimerHandle);
	}

	bBreakInProgress = false;
	bIsBroken = false;

	if (FlickerComponent)
	{
		FlickerComponent->SetStartCallsBlocked(false);
		FlickerComponent->StopAndRestore();
	}

	if (TargetLight)
	{
		if (bRestoreLightState)
		{
			TargetLight->SetIntensity(CachedOriginalIntensity);
			TargetLight->SetVisibility(bCachedOriginalVisibility);
		}
		else
		{
			TargetLight->SetIntensity(0.0f);
			TargetLight->SetVisibility(false);
		}
	}

	if (ResolvedIntactMesh)
	{
		ResolvedIntactMesh->SetVisibility(bCachedIntactMeshVisibility, true);
	}

	if (ResolvedBrokenMesh)
	{
		ResolvedBrokenMesh->SetVisibility(bCachedBrokenMeshVisibility, true);

		if (UPrimitiveComponent* BrokenMeshPrimitive = Cast<UPrimitiveComponent>(ResolvedBrokenMesh))
		{
			BrokenMeshPrimitive->SetSimulatePhysics(bCachedBrokenMeshSimulatePhysics);
			BrokenMeshPrimitive->SetEnableGravity(bCachedBrokenMeshEnableGravity);
		}
	}

}

void UBreakableLightComponent::ResolveTargetsIfNeeded()
{
	if (!bAutoResolveTargets)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!TargetLight)
	{
		TargetLight = Owner->FindComponentByClass<ULightComponent>();
	}

	if (!FlickerComponent)
	{
		FlickerComponent = Owner->FindComponentByClass<ULightFlickerModeComponent>();
	}

	if (bUseMeshSwap && (!ResolvedIntactMesh || !ResolvedBrokenMesh))
	{
		TInlineComponentArray<UMeshComponent*> MeshComponents(Owner);
		for (UMeshComponent* MeshComp : MeshComponents)
		{
			if (!MeshComp)
			{
				continue;
			}

			if (!ResolvedIntactMesh && !IntactMeshTag.IsNone() && MeshComp->ComponentHasTag(IntactMeshTag))
			{
				ResolvedIntactMesh = MeshComp;
			}

			if (!ResolvedBrokenMesh && !BrokenMeshTag.IsNone() && MeshComp->ComponentHasTag(BrokenMeshTag))
			{
				ResolvedBrokenMesh = MeshComp;
			}

			if (ResolvedIntactMesh && ResolvedBrokenMesh)
			{
				break;
			}
		}
	}
}

void UBreakableLightComponent::ApplyFinalBrokenState()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (FlickerComponent)
	{
		FlickerComponent->StopAndRestore();
		if (bBlockFlickerStartAfterBreak)
		{
			FlickerComponent->SetStartCallsBlocked(true);
		}
	}

	if (TargetLight)
	{
		TargetLight->SetIntensity(0.0f);
		TargetLight->SetVisibility(false);
	}

	if (bUseMeshSwap && ResolvedIntactMesh)
	{
		ResolvedIntactMesh->SetVisibility(false, true);
	}

	if (bUseMeshSwap && ResolvedBrokenMesh)
	{
		ResolvedBrokenMesh->SetVisibility(true, true);
		ScheduleBrokenMeshPhysics();
	}

	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BreakSound, Owner->GetActorLocation());
	}

	if (BreakFx)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			BreakFx,
			Owner->GetActorLocation() + BreakFxOffset,
			Owner->GetActorRotation(),
			FVector::OneVector,
			true,
			true,
			ENCPoolMethod::None,
			true);
	}
}

void UBreakableLightComponent::ScheduleBrokenMeshPhysics()
{
	UPrimitiveComponent* BrokenMeshPrimitive = Cast<UPrimitiveComponent>(ResolvedBrokenMesh);
	if (!BrokenMeshPrimitive)
	{
		return;
	}

	if (!bUseDelayedBrokenMeshPhysics)
	{
		ApplyBrokenMeshPhysics();
		return;
	}

	BrokenMeshPrimitive->SetSimulatePhysics(false);
	BrokenMeshPrimitive->SetEnableGravity(false);

	UWorld* World = GetWorld();
	if (!World)
	{
		ApplyBrokenMeshPhysics();
		return;
	}

	World->GetTimerManager().ClearTimer(BrokenMeshPhysicsTimerHandle);

	const float DelayMin = FMath::Max(0.0f, BrokenMeshPhysicsDelayMinSeconds);
	const float DelayMax = FMath::Max(DelayMin, BrokenMeshPhysicsDelayMaxSeconds);
	const float DelaySeconds = FMath::FRandRange(DelayMin, DelayMax);

	World->GetTimerManager().SetTimer(
		BrokenMeshPhysicsTimerHandle,
		this,
		&UBreakableLightComponent::ApplyBrokenMeshPhysics,
		DelaySeconds,
		false);
}

void UBreakableLightComponent::ApplyBrokenMeshPhysics()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BrokenMeshPhysicsTimerHandle);
	}

	UPrimitiveComponent* BrokenMeshPrimitive = Cast<UPrimitiveComponent>(ResolvedBrokenMesh);
	if (!BrokenMeshPrimitive)
	{
		return;
	}

	BrokenMeshPrimitive->SetEnableGravity(true);
	BrokenMeshPrimitive->SetSimulatePhysics(true);
}
