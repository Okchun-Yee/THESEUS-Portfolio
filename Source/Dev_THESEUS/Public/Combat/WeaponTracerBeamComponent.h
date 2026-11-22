// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "WeaponTracerBeamComponent.generated.h"

class UNiagaraSystem;

DECLARE_LOG_CATEGORY_EXTERN(LogWeaponTracerBeam, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API UWeaponTracerBeamComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponTracerBeamComponent();

	// Camera forward (fallback: actor forward) based spawn path.
	UFUNCTION(BlueprintCallable, Category = "Weapon|Tracer|Beam")
	bool SpawnTracerBeamFromCamera(AActor* SourceActor, const FVector& StartLocation, float RangeOverride = -1.0f, AActor* InstigatorActor = nullptr);

	// Advanced API: caller provides direction directly.
	UFUNCTION(BlueprintCallable, Category = "Weapon|Tracer|Beam")
	bool SpawnTracerBeam(const FVector& StartLocation, const FVector& Direction, float Range, AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Tracer|Beam")
	void SetTracerSystem(UNiagaraSystem* NewSystem) { TracerSystem = NewSystem; }

	UFUNCTION(BlueprintCallable, Category = "Weapon|Tracer|Beam")
	void SetTracerRange(float NewRange) { DefaultTracerRange = FMath::Max(0.0f, NewRange); }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tracer|Beam")
	TObjectPtr<UNiagaraSystem> TracerSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tracer|Beam", meta = (ClampMin = "0.0"))
	float DefaultTracerRange = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tracer|Beam")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tracer|Beam")
	bool bIgnoreOwnerInTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tracer|Beam")
	bool bDrawDebugTrace = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tracer|Beam")
	FName NiagaraStartParamName = TEXT("User_Start");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tracer|Beam")
	FName NiagaraEndParamName = TEXT("User_End");

private:
	bool ResolveDirectionFromSource(const AActor* SourceActor, FVector& OutDirection) const;
};
