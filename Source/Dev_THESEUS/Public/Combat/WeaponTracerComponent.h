// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponTracerComponent.generated.h"

// 예광탄(Tracer) 전용 로그 카테고리 선언
DECLARE_LOG_CATEGORY_EXTERN(LogWeaponTracer, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTracerStateChanged, bool, bActive, int32, RemainingShots);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEV_THESEUS_API UWeaponTracerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponTracerComponent();
public:
    UFUNCTION(BlueprintCallable, Category = "Weapon|Tracer")
    void ActivateTracer(int32 Shots);

    UFUNCTION(BlueprintCallable, Category = "Weapon|Tracer")
    void DeactivateTracer();

    UFUNCTION(BlueprintCallable, Category = "Weapon|Tracer")
    bool ShouldUseTracer() const;

    // 실제 발사 성공 시점에 호출
    UFUNCTION(BlueprintCallable, Category = "Weapon|Tracer")
    void OnShotFired(bool bWasTracer);

    UFUNCTION(BlueprintCallable, Category = "Weapon|Tracer")
    bool IsTracerActive() const { return bTracerActive; }

    UFUNCTION(BlueprintCallable, Category = "Weapon|Tracer")
    int32 GetRemainingTracerShots() const { return RemainingTracerShots; }

    UPROPERTY(BlueprintAssignable, Category = "Weapon|Tracer")
    FOnTracerStateChanged OnTracerStateChanged;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Tracer", meta = (AllowPrivateAccess = "true"))
    bool bTracerActive = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Tracer", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
    int32 RemainingTracerShots = 0;

    void BroadcastTracerState();
};
