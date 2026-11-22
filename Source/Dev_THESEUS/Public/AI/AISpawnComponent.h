// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AISpawnComponent.generated.h"

class AAIController;
class APawn;
class AActor;
class USplineComponent;

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API UAISpawnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAISpawnComponent();

	UFUNCTION(BlueprintCallable, Category="AI|Spawn")
	void Spawn();

	UFUNCTION(BlueprintCallable, Category="AI|Spawn")
	const TArray<APawn*>& GetSpawnedUnits() const { return Units; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	int32 ResolveSpawnCountForIndex(int32 Index) const;
	bool ResolveDropFM1024ForIndex(int32 Index) const;
	bool ResolveStartOnSplineForIndex(int32 Index) const;
	bool ResolveRequireFacingToFireForIndex(int32 Index) const;
	bool ResolveAttackRangeOverrideForIndex(int32 Index, float& OutAttackRange) const;
	bool ResolveEnableSuicideOnProximityForIndex(int32 Index) const;
	float ResolveSuicideDetonationDistanceForIndex(int32 Index) const;
	bool ResolveUseJamTriggerForIndex(int32 Index) const;
	int32 ResolveJamAfterShotsForIndex(int32 Index) const;
	bool ResolveInitialReceivePointDamageForIndex(int32 Index) const;
	USplineComponent* ResolveSplineRefFromPointActor(AActor* SpawnPointActor, int32 SpawnIndex) const;
	bool ResolveStartOnSplineFromPointActor(AActor* SpawnPointActor, int32 SpawnIndex, bool& bOutHasOverride) const;
	bool ResolveRequireFacingToFireFromPointActor(AActor* SpawnPointActor, int32 SpawnIndex, bool& bOutHasOverride) const;
	bool ResolveAttackRangeOverrideFromPointActor(AActor* SpawnPointActor, int32 SpawnIndex, float& OutAttackRange) const;
	AActor* ResolveSequence17TriggerFromPointActor(AActor* SpawnPointActor, int32 SpawnIndex) const;
	void TryInjectSplineRef(APawn* SpawnedPawn, USplineComponent* InSplineRef) const;
	void TryInjectStartOnSpline(APawn* SpawnedPawn, bool bStartOnSpline) const;
	void TryInjectStartOnSplineBlackboard(AAIController* SpawnedController, bool bStartOnSpline) const;
	void TryInjectRequireFacingToFire(APawn* SpawnedPawn, bool bRequireFacingToFire) const;
	void TryInjectRequireFacingToFireBlackboard(AAIController* SpawnedController, bool bRequireFacingToFire) const;
	void TryInjectAttackRangeBlackboard(AAIController* SpawnedController, float AttackRange) const;
	void TryInjectDropFM1024(APawn* SpawnedPawn, int32 SpawnPointIndex) const;
	void TryInjectSuicideProximitySettings(APawn* SpawnedPawn, int32 SpawnPointIndex) const;
	void TryInjectSequence17TriggerRef(APawn* SpawnedPawn, AActor* InTriggerRef) const;
	void TryConfigurePeekPatrol(APawn* SpawnedPawn, AActor* SpawnPointActor, int32 SpawnIndex) const;
	bool TryInjectJamSettings(APawn* SpawnedPawn, AActor* SpawnPointActor, int32 SpawnIndex, int32 SpawnPointIndex, bool bLogIfNotFound = true) const;
	void ScheduleJamSettingsRetry(APawn* SpawnedPawn, AActor* SpawnPointActor, int32 SpawnIndex, int32 SpawnPointIndex, int32 AttemptsLeft) const;
	void TryInjectInitialReceivePointDamage(APawn* SpawnedPawn, AActor* SpawnPointActor, int32 SpawnIndex, int32 SpawnPointIndex) const;
	bool TryApplyReceivePointDamageOnActor(AActor* TargetActor, bool bEnabled) const;

private:
	UPROPERTY(EditAnywhere, Category="AI|Spawn")
	TSubclassOf<APawn> SpawnClass;

	UPROPERTY(EditAnywhere, Category="AI|Spawn")
	TSubclassOf<AAIController> AIControllerClass;

	UPROPERTY(EditAnywhere, Category="AI|Spawn")
	TArray<FTransform> LocalTransforms;

	UPROPERTY(EditAnywhere, Category="AI|Spawn")
	TArray<TObjectPtr<AActor>> PointUnits;

	UPROPERTY(EditAnywhere, Category="AI|Spawn", meta=(ClampMin="1"))
	int32 SpawnCountPerPoint = 1;

	// If index is valid and value > 0, this value overrides SpawnCountPerPoint for that spawn point.
	UPROPERTY(EditAnywhere, Category="AI|Spawn", meta=(ClampMin="0"))
	TArray<int32> SpawnCountOverrides;

	UPROPERTY(EditAnywhere, Category="AI|Spawn", meta=(ClampMin="0.0"))
	float SpawnSpacing = 80.0f;

	// Default FM1024 drop flag for spawned AI.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Drop")
	bool bDefaultDropFM1024 = false;

	// Per spawn point override. If index is valid, this value overrides bDefaultDropFM1024.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Drop")
	TArray<bool> DropFM1024Overrides;

	// Default: spawned AI starts in non-spline mode.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Spline")
	bool bDefaultStartOnSpline = false;

	// Per spawn point override. If index is valid, this value overrides bDefaultStartOnSpline.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Spline")
	TArray<bool> StartOnSplineOverrides;

	// Default: spawned AI can attack only when facing target.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Combat")
	bool bDefaultRequireFacingToFire = true;

	// Per spawn point override. If index is valid, this value overrides bDefaultRequireFacingToFire.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Combat")
	TArray<bool> RequireFacingToFireOverrides;

	// If enabled, spawned AI Blackboard AttackRange is overridden. Set to 0 to block attack range checks.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Combat")
	bool bOverrideAttackRange = false;

	// Default Blackboard AttackRange override used when bOverrideAttackRange is enabled.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Combat", meta=(ClampMin="0.0"))
	float DefaultAttackRangeOverride = 0.0f;

	// Per spawn point override. If index is valid and value >= 0, this overrides DefaultAttackRangeOverride.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Combat", meta=(ClampMin="0.0"))
	TArray<float> AttackRangeOverrides;

	// Default: close-range suicide behavior disabled.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Suicide")
	bool bDefaultEnableSuicideOnProximity = false;

	// Per spawn point override. If index is valid, this value overrides bDefaultEnableSuicideOnProximity.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Suicide")
	TArray<bool> EnableSuicideOnProximityOverrides;

	// Default detonation distance (cm) for close-range suicide behavior.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Suicide", meta=(ClampMin="0.0"))
	float DefaultSuicideDetonationDistance = 220.0f;

	// Per spawn point override. If index is valid and value > 0, this value overrides DefaultSuicideDetonationDistance.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Suicide", meta=(ClampMin="0.0"))
	TArray<float> SuicideDetonationDistanceOverrides;

	// Default: spawned AI JAM trigger off.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Jam")
	bool bDefaultUseJamTrigger = false;

	// Per spawn point override. If index is valid, this value overrides bDefaultUseJamTrigger.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Jam")
	TArray<bool> UseJamTriggerOverrides;

	// Default JAM threshold (shots) for spawned AI.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Jam", meta=(ClampMin="1"))
	int32 DefaultJamAfterShots = 13;

	// Per spawn point override. If index is valid and value > 0, this value overrides DefaultJamAfterShots.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Jam", meta=(ClampMin="0"))
	TArray<int32> JamAfterShotsOverrides;

	// Default: spawned AI starts with receive-point-damage enabled.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Damage")
	bool bDefaultInitialReceivePointDamage = true;

	// Per spawn point override. If index is valid, this value overrides bDefaultInitialReceivePointDamage.
	UPROPERTY(EditAnywhere, Category="AI|Spawn|Damage")
	TArray<bool> InitialReceivePointDamageOverrides;

	UPROPERTY(VisibleAnywhere, Category="AI|Spawn")
	TArray<APawn*> Units;
};
