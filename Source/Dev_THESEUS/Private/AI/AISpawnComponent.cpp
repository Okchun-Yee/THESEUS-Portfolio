// Copyright 2026, Okchun-Yee.

#include "AI/AISpawnComponent.h"
#include "AI/AIPeekPatrolComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Combat/WeaponJamComponent.h"
#include "Components/ActorComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

UAISpawnComponent::UAISpawnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAISpawnComponent::Spawn()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AISpawn] Spawn skipped: client world."));
		return;
	}

	if (PointUnits.Num() <= 0 && LocalTransforms.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AISpawn] Spawn skipped: PointUnits and LocalTransforms are both empty."));
		return;
	}

	if (!SpawnClass || !AIControllerClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AISpawn] Spawn skipped: SpawnClass or AIControllerClass is null."));
		return;
	}

	const int32 NumPoints = PointUnits.Num() > 0 ? PointUnits.Num() : LocalTransforms.Num();
	for (int32 PointIndex = 0; PointIndex < NumPoints; ++PointIndex)
	{
		AActor* SpawnPointActor = PointUnits.IsValidIndex(PointIndex) ? PointUnits[PointIndex] : nullptr;
		FTransform BaseTransform = FTransform::Identity;
		if (SpawnPointActor)
		{
			BaseTransform = SpawnPointActor->GetActorTransform();
		}
		else if (LocalTransforms.IsValidIndex(PointIndex))
		{
			// Backward compatibility fallback if point actor isn't configured.
			BaseTransform = LocalTransforms[PointIndex];
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[AISpawn] Skipping point %d: no PointUnit and no LocalTransform fallback."), PointIndex);
			continue;
		}

		const int32 CountToSpawn = ResolveSpawnCountForIndex(PointIndex);

		for (int32 SpawnIndex = 0; SpawnIndex < CountToSpawn; ++SpawnIndex)
		{
			FTransform SpawnTransform = BaseTransform;
			if (SpawnSpacing > 0.0f && SpawnIndex > 0)
			{
				const FVector Right = BaseTransform.GetRotation().GetRightVector();
				SpawnTransform.AddToTranslation(Right * SpawnSpacing * static_cast<float>(SpawnIndex));
			}

			FActorSpawnParameters ControllerSpawnParams;
			ControllerSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AAIController* SpawnedController = World->SpawnActor<AAIController>(
				AIControllerClass,
				SpawnTransform.GetLocation(),
				SpawnTransform.GetRotation().Rotator(),
				ControllerSpawnParams);

			if (!SpawnedController)
			{
				UE_LOG(LogTemp, Warning, TEXT("[AISpawn] Failed to spawn AIController at point %d, unit %d."), PointIndex, SpawnIndex);
				continue;
			}

			FActorSpawnParameters PawnSpawnParams;
			PawnSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			APawn* SpawnedPawn = World->SpawnActor<APawn>(
				SpawnClass,
				SpawnTransform.GetLocation(),
				SpawnTransform.GetRotation().Rotator(),
				PawnSpawnParams);

			if (!SpawnedPawn)
			{
				UE_LOG(LogTemp, Warning, TEXT("[AISpawn] Failed to spawn pawn at point %d, unit %d."), PointIndex, SpawnIndex);
				SpawnedController->Destroy();
				continue;
			}

			USplineComponent* SplineRef = ResolveSplineRefFromPointActor(SpawnPointActor, SpawnIndex);
			bool bHasStartOnSplineOverride = false;
			bool bStartOnSpline = ResolveStartOnSplineFromPointActor(SpawnPointActor, SpawnIndex, bHasStartOnSplineOverride);
			if (!bHasStartOnSplineOverride)
			{
				bStartOnSpline = ResolveStartOnSplineForIndex(PointIndex);
			}
			bool bHasRequireFacingOverride = false;
			bool bRequireFacingToFire = ResolveRequireFacingToFireFromPointActor(SpawnPointActor, SpawnIndex, bHasRequireFacingOverride);
			if (!bHasRequireFacingOverride)
			{
				bRequireFacingToFire = ResolveRequireFacingToFireForIndex(PointIndex);
			}
			float AttackRangeOverride = 0.0f;
			bool bHasAttackRangeOverride = ResolveAttackRangeOverrideFromPointActor(SpawnPointActor, SpawnIndex, AttackRangeOverride);
			if (!bHasAttackRangeOverride)
			{
				bHasAttackRangeOverride = ResolveAttackRangeOverrideForIndex(PointIndex, AttackRangeOverride);
			}
			AActor* Sequence17TriggerRef = ResolveSequence17TriggerFromPointActor(SpawnPointActor, SpawnIndex);
			TryInjectSplineRef(SpawnedPawn, SplineRef);
			TryInjectStartOnSpline(SpawnedPawn, bStartOnSpline);
			TryInjectRequireFacingToFire(SpawnedPawn, bRequireFacingToFire);
			TryInjectDropFM1024(SpawnedPawn, PointIndex);
			TryInjectSuicideProximitySettings(SpawnedPawn, PointIndex);
			TryInjectSequence17TriggerRef(SpawnedPawn, Sequence17TriggerRef);
			SpawnedController->Possess(SpawnedPawn);
			const bool bJamInjectedNow = TryInjectJamSettings(SpawnedPawn, SpawnPointActor, SpawnIndex, PointIndex, false);
			if (!bJamInjectedNow)
			{
				ScheduleJamSettingsRetry(SpawnedPawn, SpawnPointActor, SpawnIndex, PointIndex, 12);
			}
			TryInjectInitialReceivePointDamage(SpawnedPawn, SpawnPointActor, SpawnIndex, PointIndex);
			TryInjectStartOnSplineBlackboard(SpawnedController, bStartOnSpline);
			TryInjectRequireFacingToFireBlackboard(SpawnedController, bRequireFacingToFire);
			if (bHasAttackRangeOverride)
			{
				TryInjectAttackRangeBlackboard(SpawnedController, AttackRangeOverride);
			}
			TryConfigurePeekPatrol(SpawnedPawn, SpawnPointActor, SpawnIndex);
			Units.Add(SpawnedPawn);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[AISpawn] Spawn finished: Units=%d"), Units.Num());
}

void UAISpawnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Units.Reset();
	Super::EndPlay(EndPlayReason);
}

int32 UAISpawnComponent::ResolveSpawnCountForIndex(int32 Index) const
{
	if (SpawnCountOverrides.IsValidIndex(Index) && SpawnCountOverrides[Index] > 0)
	{
		return SpawnCountOverrides[Index];
	}

	return FMath::Max(1, SpawnCountPerPoint);
}

bool UAISpawnComponent::ResolveDropFM1024ForIndex(int32 Index) const
{
	if (DropFM1024Overrides.IsValidIndex(Index))
	{
		return DropFM1024Overrides[Index];
	}

	return bDefaultDropFM1024;
}

bool UAISpawnComponent::ResolveStartOnSplineForIndex(int32 Index) const
{
	if (StartOnSplineOverrides.IsValidIndex(Index))
	{
		return StartOnSplineOverrides[Index];
	}

	return bDefaultStartOnSpline;
}

bool UAISpawnComponent::ResolveRequireFacingToFireForIndex(int32 Index) const
{
	if (RequireFacingToFireOverrides.IsValidIndex(Index))
	{
		return RequireFacingToFireOverrides[Index];
	}

	return bDefaultRequireFacingToFire;
}

bool UAISpawnComponent::ResolveAttackRangeOverrideForIndex(int32 Index, float& OutAttackRange) const
{
	if (AttackRangeOverrides.IsValidIndex(Index) && AttackRangeOverrides[Index] >= 0.0f)
	{
		OutAttackRange = AttackRangeOverrides[Index];
		return true;
	}

	if (bOverrideAttackRange)
	{
		OutAttackRange = FMath::Max(0.0f, DefaultAttackRangeOverride);
		return true;
	}

	return false;
}

bool UAISpawnComponent::ResolveEnableSuicideOnProximityForIndex(int32 Index) const
{
	if (EnableSuicideOnProximityOverrides.IsValidIndex(Index))
	{
		return EnableSuicideOnProximityOverrides[Index];
	}

	return bDefaultEnableSuicideOnProximity;
}

float UAISpawnComponent::ResolveSuicideDetonationDistanceForIndex(int32 Index) const
{
	if (SuicideDetonationDistanceOverrides.IsValidIndex(Index) && SuicideDetonationDistanceOverrides[Index] > 0.0f)
	{
		return SuicideDetonationDistanceOverrides[Index];
	}

	return FMath::Max(0.0f, DefaultSuicideDetonationDistance);
}

bool UAISpawnComponent::ResolveUseJamTriggerForIndex(int32 Index) const
{
	if (UseJamTriggerOverrides.IsValidIndex(Index))
	{
		return UseJamTriggerOverrides[Index];
	}

	return bDefaultUseJamTrigger;
}

int32 UAISpawnComponent::ResolveJamAfterShotsForIndex(int32 Index) const
{
	if (JamAfterShotsOverrides.IsValidIndex(Index) && JamAfterShotsOverrides[Index] > 0)
	{
		return FMath::Max(1, JamAfterShotsOverrides[Index]);
	}

	return FMath::Max(1, DefaultJamAfterShots);
}

bool UAISpawnComponent::ResolveInitialReceivePointDamageForIndex(int32 Index) const
{
	if (InitialReceivePointDamageOverrides.IsValidIndex(Index))
	{
		return InitialReceivePointDamageOverrides[Index];
	}

	return bDefaultInitialReceivePointDamage;
}

bool UAISpawnComponent::ResolveStartOnSplineFromPointActor(AActor* SpawnPointActor, int32 SpawnIndex, bool& bOutHasOverride) const
{
	bOutHasOverride = false;
	if (!SpawnPointActor)
	{
		return false;
	}

	auto TryReadBoolArrayByName = [SpawnPointActor, SpawnIndex, &bOutHasOverride](const TCHAR* PropertyName, bool& bOutValue) -> bool
	{
		if (FArrayProperty* BoolArrayProp = FindFProperty<FArrayProperty>(SpawnPointActor->GetClass(), PropertyName))
		{
			if (FBoolProperty* BoolInner = CastField<FBoolProperty>(BoolArrayProp->Inner))
			{
				void* ArrayData = BoolArrayProp->ContainerPtrToValuePtr<void>(SpawnPointActor);
				FScriptArrayHelper ArrayHelper(BoolArrayProp, ArrayData);
					if (ArrayHelper.IsValidIndex(SpawnIndex))
				{
					bOutValue = BoolInner->GetPropertyValue(ArrayHelper.GetRawPtr(SpawnIndex));
					bOutHasOverride = true;
					return true;
				}
			}
		}
		return false;
	};

	bool bValue = false;
	// Spawn point override policy: array-only, single canonical key.
	// Canonical key: IsSplineModeOverrides
	if (TryReadBoolArrayByName(TEXT("IsSplineModeOverrides"), bValue))
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[AISpawn] ResolveSplineMode: point=%s source=IsSplineModeOverrides[%d] value=%s"),
			*SpawnPointActor->GetName(),
			SpawnIndex,
			bValue ? TEXT("true") : TEXT("false"));
		return bValue;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[AISpawn] ResolveSplineMode: point=%s source=None"),
		*SpawnPointActor->GetName());
	return false;
}

bool UAISpawnComponent::ResolveRequireFacingToFireFromPointActor(AActor* SpawnPointActor, int32 SpawnIndex, bool& bOutHasOverride) const
{
	bOutHasOverride = false;
	if (!SpawnPointActor)
	{
		return false;
	}

	auto TryReadBoolArrayByName = [SpawnPointActor, SpawnIndex, &bOutHasOverride](const TCHAR* PropertyName, bool& bOutValue) -> bool
	{
		if (FArrayProperty* BoolArrayProp = FindFProperty<FArrayProperty>(SpawnPointActor->GetClass(), PropertyName))
		{
			if (FBoolProperty* BoolInner = CastField<FBoolProperty>(BoolArrayProp->Inner))
			{
				void* ArrayData = BoolArrayProp->ContainerPtrToValuePtr<void>(SpawnPointActor);
				FScriptArrayHelper ArrayHelper(BoolArrayProp, ArrayData);
				if (ArrayHelper.IsValidIndex(SpawnIndex))
				{
					bOutValue = BoolInner->GetPropertyValue(ArrayHelper.GetRawPtr(SpawnIndex));
					bOutHasOverride = true;
					return true;
				}
			}
		}
		return false;
	};

	bool bValue = false;
	// Spawn point override policy: array-only.
	// Scalar properties on spawn point are intentionally ignored.
	// Canonical key: RequireFacingToFireOverrides
	if (TryReadBoolArrayByName(TEXT("RequireFacingToFireOverrides"), bValue) ||
		TryReadBoolArrayByName(TEXT("bRequireFacingToFireOverrides"), bValue))
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[AISpawn] ResolveRequireFacingToFire: point=%s source=RequireFacingToFireOverrides[%d] value=%s"),
			*SpawnPointActor->GetName(),
			SpawnIndex,
			bValue ? TEXT("true") : TEXT("false"));
		return bValue;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[AISpawn] ResolveRequireFacingToFire: point=%s source=None"),
		*SpawnPointActor->GetName());
	return false;
}

bool UAISpawnComponent::ResolveAttackRangeOverrideFromPointActor(AActor* SpawnPointActor, int32 SpawnIndex, float& OutAttackRange) const
{
	if (!SpawnPointActor)
	{
		return false;
	}

	if (FArrayProperty* AttackRangeArrayProp = FindFProperty<FArrayProperty>(SpawnPointActor->GetClass(), TEXT("AttackRangeOverrides")))
	{
		if (FFloatProperty* FloatInner = CastField<FFloatProperty>(AttackRangeArrayProp->Inner))
		{
			void* ArrayData = AttackRangeArrayProp->ContainerPtrToValuePtr<void>(SpawnPointActor);
			FScriptArrayHelper ArrayHelper(AttackRangeArrayProp, ArrayData);
			if (ArrayHelper.IsValidIndex(SpawnIndex))
			{
				const float AttackRange = FloatInner->GetPropertyValue(ArrayHelper.GetRawPtr(SpawnIndex));
				if (AttackRange >= 0.0f)
				{
					OutAttackRange = AttackRange;
					UE_LOG(
						LogTemp,
						Log,
						TEXT("[AISpawn] ResolveAttackRange: point=%s source=AttackRangeOverrides[%d] value=%.1f"),
						*SpawnPointActor->GetName(),
						SpawnIndex,
						OutAttackRange);
					return true;
				}
			}
		}
		else if (FDoubleProperty* DoubleInner = CastField<FDoubleProperty>(AttackRangeArrayProp->Inner))
		{
			void* ArrayData = AttackRangeArrayProp->ContainerPtrToValuePtr<void>(SpawnPointActor);
			FScriptArrayHelper ArrayHelper(AttackRangeArrayProp, ArrayData);
			if (ArrayHelper.IsValidIndex(SpawnIndex))
			{
				const double AttackRange = DoubleInner->GetPropertyValue(ArrayHelper.GetRawPtr(SpawnIndex));
				if (AttackRange >= 0.0)
				{
					OutAttackRange = static_cast<float>(AttackRange);
					UE_LOG(
						LogTemp,
						Log,
						TEXT("[AISpawn] ResolveAttackRange: point=%s source=AttackRangeOverrides[%d] value=%.1f"),
						*SpawnPointActor->GetName(),
						SpawnIndex,
						OutAttackRange);
					return true;
				}
			}
		}
	}

	return false;
}

USplineComponent* UAISpawnComponent::ResolveSplineRefFromPointActor(AActor* SpawnPointActor, int32 SpawnIndex) const
{
	if (!SpawnPointActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AISpawn] ResolveSplineRef: SpawnPointActor is null."));
		return nullptr;
	}

	// Preferred path: read BP spawn point actor variable named "SplineRefs" (Actor reference array).
	if (FArrayProperty* SplineRefsProp = FindFProperty<FArrayProperty>(SpawnPointActor->GetClass(), TEXT("SplineRefs")))
	{
		if (FObjectProperty* SplineRefsInnerObject = CastField<FObjectProperty>(SplineRefsProp->Inner))
		{
			void* SplineRefsArrayData = SplineRefsProp->ContainerPtrToValuePtr<void>(SpawnPointActor);
			FScriptArrayHelper SplineRefsHelper(SplineRefsProp, SplineRefsArrayData);

			if (SplineRefsHelper.IsValidIndex(SpawnIndex))
			{
				UObject* SplineRefsObject = SplineRefsInnerObject->GetObjectPropertyValue(SplineRefsHelper.GetRawPtr(SpawnIndex));
				if (AActor* SplineActor = Cast<AActor>(SplineRefsObject))
				{
					if (USplineComponent* SplineFromActor = SplineActor->FindComponentByClass<USplineComponent>())
					{
						UE_LOG(
							LogTemp,
							Log,
							TEXT("[AISpawn] ResolveSplineRef: point=%s spawnIndex=%d splineRefsActor=%s spline=%s"),
							*SpawnPointActor->GetName(),
							SpawnIndex,
							*SplineActor->GetName(),
							*SplineFromActor->GetName());
						return SplineFromActor;
					}

					UE_LOG(
						LogTemp,
						Warning,
						TEXT("[AISpawn] ResolveSplineRef: SplineRefs actor %s has no spline component."),
						*SplineActor->GetName());
				}
				else if (SplineRefsObject)
				{
					UE_LOG(
						LogTemp,
						Warning,
						TEXT("[AISpawn] ResolveSplineRef: SplineRefs[%d] on %s is not an Actor (%s)."),
						SpawnIndex,
						*SpawnPointActor->GetName(),
						*SplineRefsObject->GetClass()->GetName());
				}
				else
				{
					UE_LOG(
						LogTemp,
						Warning,
						TEXT("[AISpawn] ResolveSplineRef: SplineRefs[%d] is null on %s."),
						SpawnIndex,
						*SpawnPointActor->GetName());
				}
			}
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[AISpawn] ResolveSplineRef: SplineRefs on %s is not an object array."),
				*SpawnPointActor->GetName());
		}
	}

	// Preferred path: read BP spawn point actor variable named "SplineRef" (Actor reference).
	if (FObjectProperty* SplineRefProp = FindFProperty<FObjectProperty>(SpawnPointActor->GetClass(), TEXT("SplineRef")))
	{
		UObject* SplineRefObject = SplineRefProp->GetObjectPropertyValue_InContainer(SpawnPointActor);
		if (AActor* SplineActor = Cast<AActor>(SplineRefObject))
		{
			if (USplineComponent* SplineFromActor = SplineActor->FindComponentByClass<USplineComponent>())
			{
				UE_LOG(
					LogTemp,
					Log,
					TEXT("[AISpawn] ResolveSplineRef: point=%s splineActor=%s spline=%s"),
					*SpawnPointActor->GetName(),
					*SplineActor->GetName(),
					*SplineFromActor->GetName());
				return SplineFromActor;
			}

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[AISpawn] ResolveSplineRef: SplineRef actor %s has no spline component."),
				*SplineActor->GetName());
		}
		else if (SplineRefObject)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[AISpawn] ResolveSplineRef: SplineRef on %s is not an Actor (%s)."),
				*SpawnPointActor->GetName(),
				*SplineRefObject->GetClass()->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[AISpawn] ResolveSplineRef: SplineRef is null on %s."), *SpawnPointActor->GetName());
		}
	}

	USplineComponent* SplineComp = SpawnPointActor->FindComponentByClass<USplineComponent>();
	if (!SplineComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AISpawn] ResolveSplineRef: no spline component on point actor %s."), *SpawnPointActor->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[AISpawn] ResolveSplineRef: point=%s spline=%s"), *SpawnPointActor->GetName(), *SplineComp->GetName());
	}

	return SplineComp;
}

AActor* UAISpawnComponent::ResolveSequence17TriggerFromPointActor(AActor* SpawnPointActor, int32 SpawnIndex) const
{
	if (!SpawnPointActor)
	{
		return nullptr;
	}

	// Preferred path: read BP spawn point variable named "Sequence17TriggerRefs" (Actor reference array).
	if (FArrayProperty* TriggerRefsProp = FindFProperty<FArrayProperty>(SpawnPointActor->GetClass(), TEXT("Sequence17TriggerRefs")))
	{
		if (FObjectProperty* TriggerRefsInnerObject = CastField<FObjectProperty>(TriggerRefsProp->Inner))
		{
			void* TriggerRefsArrayData = TriggerRefsProp->ContainerPtrToValuePtr<void>(SpawnPointActor);
			FScriptArrayHelper TriggerRefsHelper(TriggerRefsProp, TriggerRefsArrayData);

			if (TriggerRefsHelper.IsValidIndex(SpawnIndex))
			{
				UObject* TriggerRefsObject = TriggerRefsInnerObject->GetObjectPropertyValue(TriggerRefsHelper.GetRawPtr(SpawnIndex));
				if (AActor* TriggerActor = Cast<AActor>(TriggerRefsObject))
				{
					return TriggerActor;
				}
			}
		}
	}

	// Preferred path: read BP spawn point variable named "Sequence17TriggerRef" (Actor reference).
	if (FObjectProperty* TriggerRefProp = FindFProperty<FObjectProperty>(SpawnPointActor->GetClass(), TEXT("Sequence17TriggerRef")))
	{
		UObject* TriggerRefObject = TriggerRefProp->GetObjectPropertyValue_InContainer(SpawnPointActor);
		if (AActor* TriggerActor = Cast<AActor>(TriggerRefObject))
		{
			return TriggerActor;
		}
	}

	return nullptr;
}

void UAISpawnComponent::TryInjectSplineRef(APawn* SpawnedPawn, USplineComponent* InSplineRef) const
{
	if (!SpawnedPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AISpawn] TryInjectSplineRef skipped: Pawn=None"));
		return;
	}

	UFunction* SetterFunc = SpawnedPawn->FindFunction(TEXT("GameSetJamSplineRef"));
	if (!SetterFunc)
	{
		SetterFunc = SpawnedPawn->FindFunction(TEXT("Set Spline Ref"));
	}

	if (!SetterFunc)
	{
		SetterFunc = SpawnedPawn->FindFunction(TEXT("Game - SetJamSplineRef"));
	}

	if (!SetterFunc)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AISpawn] Spline setter not found on %s."), *SpawnedPawn->GetClass()->GetName());
		return;
	}

	struct FSetJamSplineRefParams
	{
		USplineComponent* InSplineRef = nullptr;
	};

	FSetJamSplineRefParams Params;
	Params.InSplineRef = InSplineRef;
	SpawnedPawn->ProcessEvent(SetterFunc, &Params);

	if (InSplineRef)
	{
		UE_LOG(LogTemp, Log, TEXT("[AISpawn] Injected SplineRef: Pawn=%s Spline=%s"),
			*SpawnedPawn->GetName(),
			*InSplineRef->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[AISpawn] Cleared SplineRef: Pawn=%s (no valid spline assigned)"),
			*SpawnedPawn->GetName());
	}
}

void UAISpawnComponent::TryInjectStartOnSpline(APawn* SpawnedPawn, bool bStartOnSpline) const
{
	if (!SpawnedPawn)
	{
		return;
	}

	bool bApplied = false;
	bool bAppliedIsSplineMode = false;

	// Preferred path: BP function injection.
	{
		UFunction* SetterFunc = SpawnedPawn->FindFunction(TEXT("GameSetIsSplineMode"));
		if (!SetterFunc)
		{
			SetterFunc = SpawnedPawn->FindFunction(TEXT("SetIsSplineMode"));
		}
		if (!SetterFunc)
		{
			SetterFunc = SpawnedPawn->FindFunction(TEXT("Game - SetIsSplineMode"));
		}
		if (!SetterFunc)
		{
			SetterFunc = SpawnedPawn->FindFunction(TEXT("GameSetStartOnSpline"));
		}
		if (!SetterFunc)
		{
			SetterFunc = SpawnedPawn->FindFunction(TEXT("SetStartOnSpline"));
		}
		if (!SetterFunc)
		{
			SetterFunc = SpawnedPawn->FindFunction(TEXT("Game - SetStartOnSpline"));
		}
		if (!SetterFunc)
		{
			SetterFunc = SpawnedPawn->FindFunction(TEXT("Set Spline Mode"));
		}

		if (SetterFunc)
		{
			struct FSetSplineModeParams
			{
				bool bInSplineMode = false;
			};

			FSetSplineModeParams Params;
			Params.bInSplineMode = bStartOnSpline;
			SpawnedPawn->ProcessEvent(SetterFunc, &Params);
			bApplied = true;
		}
	}

	// Primary target: IsSplineMode on AI pawn.
	if (FBoolProperty* SplineModeProp = FindFProperty<FBoolProperty>(SpawnedPawn->GetClass(), TEXT("IsSplineMode")))
	{
		SplineModeProp->SetPropertyValue_InContainer(SpawnedPawn, bStartOnSpline);
		bAppliedIsSplineMode = true;
		bApplied = true;
	}
	else if (FBoolProperty* SplineModeAltProp = FindFProperty<FBoolProperty>(SpawnedPawn->GetClass(), TEXT("bIsSplineMode")))
	{
		SplineModeAltProp->SetPropertyValue_InContainer(SpawnedPawn, bStartOnSpline);
		bAppliedIsSplineMode = true;
		bApplied = true;
	}

	// Backward-compatibility write-through to legacy property names.
	if (FBoolProperty* StartOnSplineProp = FindFProperty<FBoolProperty>(SpawnedPawn->GetClass(), TEXT("StartOnSpline")))
	{
		StartOnSplineProp->SetPropertyValue_InContainer(SpawnedPawn, bStartOnSpline);
		bApplied = true;
	}
	if (FBoolProperty* StartOnSplineAltProp = FindFProperty<FBoolProperty>(SpawnedPawn->GetClass(), TEXT("bStartOnSpline")))
	{
		StartOnSplineAltProp->SetPropertyValue_InContainer(SpawnedPawn, bStartOnSpline);
		bApplied = true;
	}

	if (bApplied)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[AISpawn] Injected SplineMode: Pawn=%s IsSplineMode=%s IsSplineModeApplied=%s"),
			*SpawnedPawn->GetName(),
			bStartOnSpline ? TEXT("true") : TEXT("false"),
			bAppliedIsSplineMode ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AISpawn] SplineMode target not found on %s (expected function SetIsSplineMode/SetStartOnSpline or bool property IsSplineMode)."),
			*SpawnedPawn->GetClass()->GetName());
	}
}

void UAISpawnComponent::TryInjectStartOnSplineBlackboard(AAIController* SpawnedController, bool bStartOnSpline) const
{
	if (!SpawnedController)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = SpawnedController->GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return;
	}

	BlackboardComp->SetValueAsBool(TEXT("IsSplineMode"), bStartOnSpline);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[AISpawn] Injected BB IsSplineMode: Controller=%s Value=%s"),
		*SpawnedController->GetName(),
		bStartOnSpline ? TEXT("true") : TEXT("false"));
}

void UAISpawnComponent::TryInjectRequireFacingToFire(APawn* SpawnedPawn, bool bRequireFacingToFire) const
{
	if (!SpawnedPawn)
	{
		return;
	}

	bool bApplied = false;

	{
		UFunction* SetterFunc = SpawnedPawn->FindFunction(TEXT("GameSetRequireFacingToFire"));
		if (!SetterFunc)
		{
			SetterFunc = SpawnedPawn->FindFunction(TEXT("SetRequireFacingToFire"));
		}
		if (!SetterFunc)
		{
			SetterFunc = SpawnedPawn->FindFunction(TEXT("Game - SetRequireFacingToFire"));
		}

		if (SetterFunc)
		{
			struct FSetRequireFacingToFireParams
			{
				bool bInRequireFacingToFire = false;
			};

			FSetRequireFacingToFireParams Params;
			Params.bInRequireFacingToFire = bRequireFacingToFire;
			SpawnedPawn->ProcessEvent(SetterFunc, &Params);
			bApplied = true;
		}
	}

	if (FBoolProperty* RequireFacingProp = FindFProperty<FBoolProperty>(SpawnedPawn->GetClass(), TEXT("RequireFacingToFire")))
	{
		RequireFacingProp->SetPropertyValue_InContainer(SpawnedPawn, bRequireFacingToFire);
		bApplied = true;
	}
	else if (FBoolProperty* RequireFacingAltProp = FindFProperty<FBoolProperty>(SpawnedPawn->GetClass(), TEXT("bRequireFacingToFire")))
	{
		RequireFacingAltProp->SetPropertyValue_InContainer(SpawnedPawn, bRequireFacingToFire);
		bApplied = true;
	}

	if (bApplied)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[AISpawn] Injected RequireFacingToFire: Pawn=%s Value=%s"),
			*SpawnedPawn->GetName(),
			bRequireFacingToFire ? TEXT("true") : TEXT("false"));
	}
}

void UAISpawnComponent::TryInjectRequireFacingToFireBlackboard(AAIController* SpawnedController, bool bRequireFacingToFire) const
{
	if (!SpawnedController)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = SpawnedController->GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return;
	}

	BlackboardComp->SetValueAsBool(TEXT("RequireFacingToFire"), bRequireFacingToFire);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[AISpawn] Injected BB RequireFacingToFire: Controller=%s Value=%s"),
		*SpawnedController->GetName(),
		bRequireFacingToFire ? TEXT("true") : TEXT("false"));
}

void UAISpawnComponent::TryInjectAttackRangeBlackboard(AAIController* SpawnedController, float AttackRange) const
{
	if (!SpawnedController)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = SpawnedController->GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return;
	}

	const float ClampedAttackRange = FMath::Max(0.0f, AttackRange);
	bool bAppliedAttackRange = false;
	bool bAppliedPrefixedAttackRange = false;

	const auto TrySetAttackRangeKey = [BlackboardComp, ClampedAttackRange](const FName KeyName) -> bool
	{
		const auto KeyId = BlackboardComp->GetKeyID(KeyName);
		if (!BlackboardComp->IsValidKey(KeyId))
		{
			return false;
		}

		BlackboardComp->SetValueAsFloat(KeyName, ClampedAttackRange);
		return true;
	};

	bAppliedAttackRange = TrySetAttackRangeKey(TEXT("AttackRange"));
	bAppliedPrefixedAttackRange = TrySetAttackRangeKey(TEXT("T_AttackRange"));

	if (!bAppliedAttackRange && !bAppliedPrefixedAttackRange)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AISpawn] AttackRange blackboard key not found: Controller=%s"),
			*SpawnedController->GetName());
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[AISpawn] Injected BB AttackRange: Controller=%s Value=%.1f Keys=%s%s"),
		*SpawnedController->GetName(),
		ClampedAttackRange,
		bAppliedAttackRange ? TEXT("AttackRange") : TEXT(""),
		bAppliedPrefixedAttackRange ? TEXT(" T_AttackRange") : TEXT(""));
}

void UAISpawnComponent::TryInjectDropFM1024(APawn* SpawnedPawn, int32 SpawnPointIndex) const
{
	if (!SpawnedPawn)
	{
		return;
	}

	const bool bShouldDropFM1024 = ResolveDropFM1024ForIndex(SpawnPointIndex);
	bool bAppliedDropFlag = false;

	if (FBoolProperty* DropFlagProp = FindFProperty<FBoolProperty>(SpawnedPawn->GetClass(), TEXT("bisDropsFM1024")))
	{
		DropFlagProp->SetPropertyValue_InContainer(SpawnedPawn, bShouldDropFM1024);
		bAppliedDropFlag = true;
	}
	else if (FBoolProperty* DropFlagAltProp = FindFProperty<FBoolProperty>(SpawnedPawn->GetClass(), TEXT("bIsDropsFM1024")))
	{
		DropFlagAltProp->SetPropertyValue_InContainer(SpawnedPawn, bShouldDropFM1024);
		bAppliedDropFlag = true;
	}
	else if (FBoolProperty* DropFlagAlt2Prop = FindFProperty<FBoolProperty>(SpawnedPawn->GetClass(), TEXT("bDropsFM1024")))
	{
		DropFlagAlt2Prop->SetPropertyValue_InContainer(SpawnedPawn, bShouldDropFM1024);
		bAppliedDropFlag = true;
	}

	if (FBoolProperty* CanDroppedProp = FindFProperty<FBoolProperty>(SpawnedPawn->GetClass(), TEXT("bCanDroppedFM1024")))
	{
		CanDroppedProp->SetPropertyValue_InContainer(SpawnedPawn, false);
	}

	if (bAppliedDropFlag)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[AISpawn] Injected FM1024 drop flag: Pawn=%s PointIndex=%d Drop=%s"),
			*SpawnedPawn->GetName(),
			SpawnPointIndex,
			bShouldDropFM1024 ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AISpawn] FM1024 drop flag property not found on %s."),
			*SpawnedPawn->GetClass()->GetName());
	}
}

void UAISpawnComponent::TryInjectSuicideProximitySettings(APawn* SpawnedPawn, int32 SpawnPointIndex) const
{
	if (!SpawnedPawn)
	{
		return;
	}

	const bool bEnableSuicide = ResolveEnableSuicideOnProximityForIndex(SpawnPointIndex);
	const float DetonationDistance = ResolveSuicideDetonationDistanceForIndex(SpawnPointIndex);
	bool bApplied = false;

	// Preferred path: function injection on BP AI.
	{
		UFunction* SetterFunc = SpawnedPawn->FindFunction(TEXT("GameSetSuicideProximitySettings"));
		if (!SetterFunc)
		{
			SetterFunc = SpawnedPawn->FindFunction(TEXT("SetSuicideProximitySettings"));
		}
		if (!SetterFunc)
		{
			SetterFunc = SpawnedPawn->FindFunction(TEXT("Game - SetSuicideProximitySettings"));
		}

		if (SetterFunc)
		{
			struct FSetSuicideProximityParams
			{
				bool bEnableSuicideOnProximity = false;
				double InSuicideDistance = 0.0;
			};

			FSetSuicideProximityParams Params;
			Params.bEnableSuicideOnProximity = bEnableSuicide;
			Params.InSuicideDistance = static_cast<double>(DetonationDistance);
			SpawnedPawn->ProcessEvent(SetterFunc, &Params);
			bApplied = true;
		}
	}

	// Fallback path: direct property injection.
	if (FBoolProperty* EnableProp = FindFProperty<FBoolProperty>(SpawnedPawn->GetClass(), TEXT("bEnableSuicideOnProximity")))
	{
		EnableProp->SetPropertyValue_InContainer(SpawnedPawn, bEnableSuicide);
		bApplied = true;
	}
	else if (FBoolProperty* EnableAltProp = FindFProperty<FBoolProperty>(SpawnedPawn->GetClass(), TEXT("bSuicideOnProximity")))
	{
		EnableAltProp->SetPropertyValue_InContainer(SpawnedPawn, bEnableSuicide);
		bApplied = true;
	}

	if (FDoubleProperty* DistancePropDouble = FindFProperty<FDoubleProperty>(SpawnedPawn->GetClass(), TEXT("SuicideDetonationDistance")))
	{
		DistancePropDouble->SetPropertyValue_InContainer(SpawnedPawn, static_cast<double>(DetonationDistance));
		bApplied = true;
	}
	else if (FFloatProperty* DistanceProp = FindFProperty<FFloatProperty>(SpawnedPawn->GetClass(), TEXT("SuicideDetonationDistance")))
	{
		DistanceProp->SetPropertyValue_InContainer(SpawnedPawn, DetonationDistance);
		bApplied = true;
	}
	else if (FDoubleProperty* DistanceAltPropDouble = FindFProperty<FDoubleProperty>(SpawnedPawn->GetClass(), TEXT("DetonationDistance")))
	{
		DistanceAltPropDouble->SetPropertyValue_InContainer(SpawnedPawn, static_cast<double>(DetonationDistance));
		bApplied = true;
	}
	else if (FFloatProperty* DistanceAltProp = FindFProperty<FFloatProperty>(SpawnedPawn->GetClass(), TEXT("DetonationDistance")))
	{
		DistanceAltProp->SetPropertyValue_InContainer(SpawnedPawn, DetonationDistance);
		bApplied = true;
	}

	if (bApplied)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[AISpawn] Injected suicide proximity: Pawn=%s PointIndex=%d Enable=%s Distance=%.1f"),
			*SpawnedPawn->GetName(),
			SpawnPointIndex,
			bEnableSuicide ? TEXT("true") : TEXT("false"),
			DetonationDistance);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AISpawn] Suicide proximity target not found on %s (expected function SetSuicideProximitySettings or properties bEnableSuicideOnProximity/SuicideDetonationDistance)."),
			*SpawnedPawn->GetClass()->GetName());
	}
}

void UAISpawnComponent::TryInjectSequence17TriggerRef(APawn* SpawnedPawn, AActor* InTriggerRef) const
{
	if (!SpawnedPawn)
	{
		return;
	}

	// No-op by design when the spawn point does not provide a Sequence17 trigger.
	if (!InTriggerRef)
	{
		return;
	}

	if (FObjectProperty* TriggerRefProp = FindFProperty<FObjectProperty>(SpawnedPawn->GetClass(), TEXT("Sequence17TriggerRef")))
	{
		TriggerRefProp->SetObjectPropertyValue_InContainer(SpawnedPawn, InTriggerRef);
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[AISpawn] Injected Sequence17TriggerRef: Pawn=%s Trigger=%s"),
			*SpawnedPawn->GetName(),
			*InTriggerRef->GetName());
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AISpawn] Sequence17TriggerRef property not found on %s."),
			*SpawnedPawn->GetClass()->GetName());
	}
}

bool UAISpawnComponent::TryInjectJamSettings(APawn* SpawnedPawn, AActor* SpawnPointActor, int32 SpawnIndex, int32 SpawnPointIndex, bool bLogIfNotFound) const
{
	if (!SpawnedPawn)
	{
		return false;
	}

	const bool bDefaultUseJam = ResolveUseJamTriggerForIndex(SpawnPointIndex);
	const int32 DefaultJamShots = ResolveJamAfterShotsForIndex(SpawnPointIndex);

	bool bUseJamTrigger = bDefaultUseJam;
	int32 JamAfterShots = DefaultJamShots;

	// Spawn point override policy: array-only.
	// Scalar properties on spawn point are intentionally ignored.
	auto TryReadBoolArrayByName = [SpawnPointActor, SpawnIndex](const TCHAR* PropertyName, bool& OutValue) -> bool
	{
		if (!SpawnPointActor)
		{
			return false;
		}
		if (FArrayProperty* BoolArrayProp = FindFProperty<FArrayProperty>(SpawnPointActor->GetClass(), PropertyName))
		{
			if (FBoolProperty* BoolInner = CastField<FBoolProperty>(BoolArrayProp->Inner))
			{
				void* ArrayData = BoolArrayProp->ContainerPtrToValuePtr<void>(SpawnPointActor);
				FScriptArrayHelper ArrayHelper(BoolArrayProp, ArrayData);
					if (ArrayHelper.IsValidIndex(SpawnIndex))
				{
					OutValue = BoolInner->GetPropertyValue(ArrayHelper.GetRawPtr(SpawnIndex));
					return true;
				}
			}
		}
		return false;
	};

	auto TryReadNumberArrayByName = [SpawnPointActor, SpawnIndex](const TCHAR* PropertyName, int32& OutValue) -> bool
	{
		if (!SpawnPointActor)
		{
			return false;
		}
		if (FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(SpawnPointActor->GetClass(), PropertyName))
		{
			void* ArrayData = ArrayProp->ContainerPtrToValuePtr<void>(SpawnPointActor);
			FScriptArrayHelper ArrayHelper(ArrayProp, ArrayData);
			if (!ArrayHelper.IsValidIndex(SpawnIndex))
			{
				return false;
			}

			if (FIntProperty* IntInner = CastField<FIntProperty>(ArrayProp->Inner))
			{
				OutValue = IntInner->GetPropertyValue(ArrayHelper.GetRawPtr(SpawnIndex));
				return true;
			}
			if (FDoubleProperty* DoubleInner = CastField<FDoubleProperty>(ArrayProp->Inner))
			{
				OutValue = FMath::RoundToInt(static_cast<float>(DoubleInner->GetPropertyValue(ArrayHelper.GetRawPtr(SpawnIndex))));
				return true;
			}
			if (FFloatProperty* FloatInner = CastField<FFloatProperty>(ArrayProp->Inner))
			{
				OutValue = FMath::RoundToInt(FloatInner->GetPropertyValue(ArrayHelper.GetRawPtr(SpawnIndex)));
				return true;
			}
		}
		return false;
	};

	// Spawn point override policy: array-only, single canonical key.
	// Canonical key: UseJamTriggerOverrides
	TryReadBoolArrayByName(TEXT("UseJamTriggerOverrides"), bUseJamTrigger);

	// Canonical key: JamAfterShotsOverrides
	TryReadNumberArrayByName(TEXT("JamAfterShotsOverrides"), JamAfterShots);

	JamAfterShots = FMath::Max(1, JamAfterShots);

	int32 AppliedComponentCount = 0;
	auto ApplyJamOnActor = [bUseJamTrigger, JamAfterShots, &AppliedComponentCount](AActor* TargetActor) -> void
	{
		if (!IsValid(TargetActor))
		{
			return;
		}

		TArray<UWeaponJamComponent*> JamComponents;
		TargetActor->GetComponents<UWeaponJamComponent>(JamComponents);
		for (UWeaponJamComponent* JamComp : JamComponents)
		{
			if (!IsValid(JamComp))
			{
				continue;
			}

			JamComp->SetJamAfterShots(JamAfterShots);
			if (bUseJamTrigger)
			{
				JamComp->StartJamCounting(true);
			}
			else
			{
				JamComp->StopJamCounting();
				JamComp->ClearJam(true);
			}
			++AppliedComponentCount;
		}
	};

	ApplyJamOnActor(SpawnedPawn);

	TArray<AActor*> AttachedActors;
	SpawnedPawn->GetAttachedActors(AttachedActors);
	for (AActor* AttachedActor : AttachedActors)
	{
		ApplyJamOnActor(AttachedActor);
	}

	TArray<UChildActorComponent*> ChildActorComponents;
	SpawnedPawn->GetComponents<UChildActorComponent>(ChildActorComponents);
	for (UChildActorComponent* ChildComp : ChildActorComponents)
	{
		if (IsValid(ChildComp) && IsValid(ChildComp->GetChildActor()))
		{
			ApplyJamOnActor(ChildComp->GetChildActor());
		}
	}

	if (AppliedComponentCount > 0)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[AISpawn] Injected JAM settings: Pawn=%s PointIndex=%d SpawnIndex=%d UseJamTrigger=%s JamAfterShots=%d Components=%d"),
			*SpawnedPawn->GetName(),
			SpawnPointIndex,
			SpawnIndex,
			bUseJamTrigger ? TEXT("true") : TEXT("false"),
			JamAfterShots,
			AppliedComponentCount);
		return true;
	}

	if (bLogIfNotFound)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AISpawn] JAM settings target not found: Pawn=%s (no UWeaponJamComponent on pawn/attached/child actors)."),
			*SpawnedPawn->GetName());
	}

	return false;
}

void UAISpawnComponent::ScheduleJamSettingsRetry(APawn* SpawnedPawn, AActor* SpawnPointActor, int32 SpawnIndex, int32 SpawnPointIndex, int32 AttemptsLeft) const
{
	if (AttemptsLeft <= 0 || !IsValid(SpawnedPawn))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TWeakObjectPtr<UAISpawnComponent> SelfWeak(const_cast<UAISpawnComponent*>(this));
	TWeakObjectPtr<APawn> PawnWeak(SpawnedPawn);
	TWeakObjectPtr<AActor> SpawnPointWeak(SpawnPointActor);

	FTimerDelegate RetryDelegate;
	RetryDelegate.BindLambda([SelfWeak, PawnWeak, SpawnPointWeak, SpawnIndex, SpawnPointIndex, AttemptsLeft]()
	{
		UAISpawnComponent* Self = SelfWeak.Get();
		APawn* Pawn = PawnWeak.Get();
		if (!Self || !Pawn)
		{
			return;
		}

		AActor* SpawnPoint = SpawnPointWeak.Get();
		const bool bShouldLogOnFailure = (AttemptsLeft <= 1);
		const bool bInjected = Self->TryInjectJamSettings(Pawn, SpawnPoint, SpawnIndex, SpawnPointIndex, bShouldLogOnFailure);
		if (bInjected)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[AISpawn] JAM settings retry success: Pawn=%s PointIndex=%d SpawnIndex=%d AttemptsLeft=%d"),
				*Pawn->GetName(),
				SpawnPointIndex,
				SpawnIndex,
				AttemptsLeft);
			return;
		}

		Self->ScheduleJamSettingsRetry(Pawn, SpawnPoint, SpawnIndex, SpawnPointIndex, AttemptsLeft - 1);
	});

	FTimerHandle RetryHandle;
	World->GetTimerManager().SetTimer(RetryHandle, RetryDelegate, 0.1f, false);
}

void UAISpawnComponent::TryInjectInitialReceivePointDamage(APawn* SpawnedPawn, AActor* SpawnPointActor, int32 SpawnIndex, int32 SpawnPointIndex) const
{
	if (!SpawnedPawn)
	{
		return;
	}

	bool bReceivePointDamageEnabled = ResolveInitialReceivePointDamageForIndex(SpawnPointIndex);
	bool bResolvedFromSpawnPoint = false;
	const TCHAR* ResolvedSource = TEXT("SpawnerDefault");

	// Spawn point override policy: array-only.
	// Scalar properties on spawn point are intentionally ignored.
	auto TryReadBoolArrayByName = [SpawnPointActor](const TCHAR* PropertyName, int32 InIndex, bool& OutValue) -> bool
	{
		if (!SpawnPointActor)
		{
			return false;
		}
		if (FArrayProperty* BoolArrayProp = FindFProperty<FArrayProperty>(SpawnPointActor->GetClass(), PropertyName))
		{
			if (FBoolProperty* BoolInner = CastField<FBoolProperty>(BoolArrayProp->Inner))
			{
				void* ArrayData = BoolArrayProp->ContainerPtrToValuePtr<void>(SpawnPointActor);
				FScriptArrayHelper ArrayHelper(BoolArrayProp, ArrayData);
					if (ArrayHelper.IsValidIndex(InIndex))
				{
					OutValue = BoolInner->GetPropertyValue(ArrayHelper.GetRawPtr(InIndex));
					return true;
				}
			}
		}
		return false;
	};

	auto TryResolveFromSpawnPointArrays = [&](int32 InIndex) -> bool
	{
		// Spawn point override policy: array-only, single canonical key.
		// Canonical key: InitialReceivePointDamageOverrides
		if (TryReadBoolArrayByName(TEXT("InitialReceivePointDamageOverrides"), InIndex, bReceivePointDamageEnabled))
		{
			bResolvedFromSpawnPoint = true;
			return true;
		}
		return false;
	};

	// 1) per-unit override (SpawnIndex) first
	if (TryResolveFromSpawnPointArrays(SpawnIndex))
	{
		ResolvedSource = TEXT("SpawnPointArrayBySpawnIndex");
	}
	// 2) fallback by SpawnPointIndex for setups that index arrays by point slot
	else if (TryResolveFromSpawnPointArrays(SpawnPointIndex))
	{
		ResolvedSource = TEXT("SpawnPointArrayByPointIndex");
	}

	int32 AppliedTargets = 0;
	if (TryApplyReceivePointDamageOnActor(SpawnedPawn, bReceivePointDamageEnabled))
	{
		++AppliedTargets;
	}

	TArray<AActor*> AttachedActors;
	SpawnedPawn->GetAttachedActors(AttachedActors);
	for (AActor* AttachedActor : AttachedActors)
	{
		if (TryApplyReceivePointDamageOnActor(AttachedActor, bReceivePointDamageEnabled))
		{
			++AppliedTargets;
		}
	}

	TArray<UChildActorComponent*> ChildActorComponents;
	SpawnedPawn->GetComponents<UChildActorComponent>(ChildActorComponents);
	for (UChildActorComponent* ChildComp : ChildActorComponents)
	{
		if (!IsValid(ChildComp) || !IsValid(ChildComp->GetChildActor()))
		{
			continue;
		}

		if (TryApplyReceivePointDamageOnActor(ChildComp->GetChildActor(), bReceivePointDamageEnabled))
		{
			++AppliedTargets;
		}
	}

	if (AppliedTargets > 0)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[AISpawn] Injected initial receive-point-damage: Pawn=%s PointIndex=%d SpawnIndex=%d Enabled=%s Targets=%d Source=%s"),
			*SpawnedPawn->GetName(),
			SpawnPointIndex,
			SpawnIndex,
			bReceivePointDamageEnabled ? TEXT("true") : TEXT("false"),
			AppliedTargets,
			ResolvedSource);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AISpawn] Initial receive-point-damage target not found on %s (Source=%s, ResolvedFromSpawnPoint=%s)."),
			*SpawnedPawn->GetName(),
			ResolvedSource,
			bResolvedFromSpawnPoint ? TEXT("true") : TEXT("false"));
	}
}

bool UAISpawnComponent::TryApplyReceivePointDamageOnActor(AActor* TargetActor, bool bEnabled) const
{
	if (!IsValid(TargetActor))
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

	bool bApplied = false;

	for (const FName CandidateName : CandidatePropertyNames)
	{
		if (FBoolProperty* ActorBoolProp = FindFProperty<FBoolProperty>(TargetActor->GetClass(), CandidateName))
		{
			ActorBoolProp->SetPropertyValue_InContainer(TargetActor, bEnabled);
			bApplied = true;
			break;
		}
	}

	TArray<UActorComponent*> Components;
	TargetActor->GetComponents(Components);
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
		bApplied = true;
	}

	return bApplied;
}

void UAISpawnComponent::TryConfigurePeekPatrol(APawn* SpawnedPawn, AActor* SpawnPointActor, int32 SpawnIndex) const
{
	if (!SpawnedPawn)
	{
		return;
	}

	auto TryReadBoolByName = [SpawnPointActor](const TCHAR* PropertyName, bool& bOutValue) -> bool
	{
		if (!SpawnPointActor)
		{
			return false;
		}

		if (FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(SpawnPointActor->GetClass(), PropertyName))
		{
			bOutValue = BoolProp->GetPropertyValue_InContainer(SpawnPointActor);
			return true;
		}
		return false;
	};

	auto TryReadBoolArrayByName = [SpawnPointActor, SpawnIndex](const TCHAR* PropertyName, bool& bOutValue) -> bool
	{
		if (!SpawnPointActor)
		{
			return false;
		}

		if (FArrayProperty* BoolArrayProp = FindFProperty<FArrayProperty>(SpawnPointActor->GetClass(), PropertyName))
		{
			if (FBoolProperty* BoolInner = CastField<FBoolProperty>(BoolArrayProp->Inner))
			{
				void* ArrayData = BoolArrayProp->ContainerPtrToValuePtr<void>(SpawnPointActor);
				FScriptArrayHelper ArrayHelper(BoolArrayProp, ArrayData);
				if (ArrayHelper.IsValidIndex(SpawnIndex))
				{
					bOutValue = BoolInner->GetPropertyValue(ArrayHelper.GetRawPtr(SpawnIndex));
					return true;
				}
			}
		}
		return false;
	};

	auto TryReadActorByName = [SpawnPointActor](const TCHAR* PropertyName, AActor*& OutActor) -> bool
	{
		if (!SpawnPointActor)
		{
			return false;
		}

		if (FObjectProperty* ObjectProp = FindFProperty<FObjectProperty>(SpawnPointActor->GetClass(), PropertyName))
		{
			UObject* Obj = ObjectProp->GetObjectPropertyValue_InContainer(SpawnPointActor);
			if (AActor* AsActor = Cast<AActor>(Obj))
			{
				OutActor = AsActor;
				return true;
			}
		}
		return false;
	};

	auto TryReadActorArrayByName = [SpawnPointActor, SpawnIndex](const TCHAR* PropertyName, AActor*& OutActor) -> bool
	{
		if (!SpawnPointActor)
		{
			return false;
		}

		if (FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(SpawnPointActor->GetClass(), PropertyName))
		{
			if (FObjectProperty* ObjectInner = CastField<FObjectProperty>(ArrayProp->Inner))
			{
				void* ArrayData = ArrayProp->ContainerPtrToValuePtr<void>(SpawnPointActor);
				FScriptArrayHelper ArrayHelper(ArrayProp, ArrayData);
				if (ArrayHelper.IsValidIndex(SpawnIndex))
				{
					UObject* Obj = ObjectInner->GetObjectPropertyValue(ArrayHelper.GetRawPtr(SpawnIndex));
					if (AActor* AsActor = Cast<AActor>(Obj))
					{
						OutActor = AsActor;
						return true;
					}
				}
			}
		}
		return false;
	};

	auto TryReadNumberByName = [SpawnPointActor](const TCHAR* PropertyName, float& OutValue) -> bool
	{
		if (!SpawnPointActor)
		{
			return false;
		}

		if (FDoubleProperty* DoubleProp = FindFProperty<FDoubleProperty>(SpawnPointActor->GetClass(), PropertyName))
		{
			OutValue = static_cast<float>(DoubleProp->GetPropertyValue_InContainer(SpawnPointActor));
			return true;
		}
		if (FFloatProperty* FloatProp = FindFProperty<FFloatProperty>(SpawnPointActor->GetClass(), PropertyName))
		{
			OutValue = FloatProp->GetPropertyValue_InContainer(SpawnPointActor);
			return true;
		}
		return false;
	};

	auto TryReadNumberArrayByName = [SpawnPointActor, SpawnIndex](const TCHAR* PropertyName, float& OutValue) -> bool
	{
		if (!SpawnPointActor)
		{
			return false;
		}

		if (FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(SpawnPointActor->GetClass(), PropertyName))
		{
			void* ArrayData = ArrayProp->ContainerPtrToValuePtr<void>(SpawnPointActor);
			FScriptArrayHelper ArrayHelper(ArrayProp, ArrayData);
			if (!ArrayHelper.IsValidIndex(SpawnIndex))
			{
				return false;
			}

			if (FDoubleProperty* DoubleInner = CastField<FDoubleProperty>(ArrayProp->Inner))
			{
				OutValue = static_cast<float>(DoubleInner->GetPropertyValue(ArrayHelper.GetRawPtr(SpawnIndex)));
				return true;
			}
			if (FFloatProperty* FloatInner = CastField<FFloatProperty>(ArrayProp->Inner))
			{
				OutValue = FloatInner->GetPropertyValue(ArrayHelper.GetRawPtr(SpawnIndex));
				return true;
			}
		}
		return false;
	};

	bool bUsePeekPatrol = false;
	const bool bHasUsePeekPatrol =
		TryReadBoolByName(TEXT("UsePeekPatrol"), bUsePeekPatrol) ||
		TryReadBoolByName(TEXT("bUsePeekPatrol"), bUsePeekPatrol) ||
		TryReadBoolArrayByName(TEXT("UsePeekPatrolOverrides"), bUsePeekPatrol) ||
		TryReadBoolArrayByName(TEXT("bUsePeekPatrolOverrides"), bUsePeekPatrol);

	if (!bHasUsePeekPatrol || !bUsePeekPatrol)
	{
		return;
	}

	AActor* LeftPoint = nullptr;
	AActor* RightPoint = nullptr;
	float WaitTime = -1.0f;
	float TimeoutToRush = -1.0f;

	TryReadActorByName(TEXT("PeekLeftPoint"), LeftPoint) ||
		TryReadActorByName(TEXT("PeekLeftRef"), LeftPoint) ||
		TryReadActorArrayByName(TEXT("PeekLeftPoints"), LeftPoint) ||
		TryReadActorArrayByName(TEXT("PeekLeftPointRefs"), LeftPoint);

	TryReadActorByName(TEXT("PeekRightPoint"), RightPoint) ||
		TryReadActorByName(TEXT("PeekRightRef"), RightPoint) ||
		TryReadActorArrayByName(TEXT("PeekRightPoints"), RightPoint) ||
		TryReadActorArrayByName(TEXT("PeekRightPointRefs"), RightPoint);

	TryReadNumberByName(TEXT("PeekWaitTime"), WaitTime) ||
		TryReadNumberArrayByName(TEXT("PeekWaitTimeOverrides"), WaitTime);

	TryReadNumberByName(TEXT("PeekTimeoutToRush"), TimeoutToRush) ||
		TryReadNumberArrayByName(TEXT("PeekTimeoutToRushOverrides"), TimeoutToRush);

	UAIPeekPatrolComponent* PeekComponent = SpawnedPawn->FindComponentByClass<UAIPeekPatrolComponent>();
	if (!PeekComponent)
	{
		PeekComponent = NewObject<UAIPeekPatrolComponent>(SpawnedPawn, UAIPeekPatrolComponent::StaticClass(), TEXT("AIPeekPatrolComponent"));
		if (PeekComponent)
		{
			PeekComponent->RegisterComponent();
		}
	}

	if (!PeekComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AISpawn] PeekPatrol setup failed: component create failed. Pawn=%s"), *SpawnedPawn->GetName());
		return;
	}

	PeekComponent->ConfigurePeekPatrol(LeftPoint, RightPoint, WaitTime, TimeoutToRush);
	const bool bStarted = PeekComponent->StartPeekPatrol();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[AISpawn] PeekPatrol injected: Pawn=%s Started=%s Left=%s Right=%s Wait=%.2f Timeout=%.2f"),
		*SpawnedPawn->GetName(),
		bStarted ? TEXT("true") : TEXT("false"),
		*GetNameSafe(LeftPoint),
		*GetNameSafe(RightPoint),
		WaitTime,
		TimeoutToRush);
}
