// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "WeaponJamComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWeaponJam, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJamStateChanged, bool, bNewJammed, int32, CurrentShotCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJamCountingChanged, bool, bEnabled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJamSplineEventRequested, int32, CurrentShotCount);

class APawn;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API UWeaponJamComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponJamComponent();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Jam")
	void StartJamCounting(bool bResetShotCount = true);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Jam")
	void StopJamCounting();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Jam")
	bool IsCountingEnabled() const { return bCountingEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Weapon|Jam")
	bool CanFire() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Jam")
	void RegisterSuccessfulShot();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Jam")
	void ClearJam(bool bResetShotCount = true);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Jam")
	int32 GetShotCount() const { return ShotCount; }

	UFUNCTION(BlueprintCallable, Category = "Weapon|Jam")
	bool IsJammed() const { return bIsJammed; }

	UFUNCTION(BlueprintCallable, Category = "Weapon|Jam")
	void SetJamAfterShots(int32 NewThreshold);

	/** ShotCount/임계값과 무관하게 즉시 JAM 상태로 전환한다. */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Jam")
	void ForceJam();

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Jam")
	FOnJamStateChanged OnJamStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Jam")
	FOnJamCountingChanged OnJamCountingChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Jam")
	FOnJamSplineEventRequested OnJamSplineEventRequested;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Jam", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 JamAfterShots = 13;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Jam", meta = (AllowPrivateAccess = "true"))
	int32 ShotCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Jam", meta = (AllowPrivateAccess = "true"))
	bool bIsJammed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Jam", meta = (AllowPrivateAccess = "true"))
	bool bCountingEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Jam|AI", meta = (AllowPrivateAccess = "true"))
	bool bEnableJamSplineEvent = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Jam|AI", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float JamSplineEventDelay = 7.0f;

	/** AI가 JAM 상태가 될 때 플레이어 무기에도 JAM 상태를 전파할지 여부 (SplineEvent 사용 시 해당 시점, 미사용 시 즉시) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Jam|AI", meta = (AllowPrivateAccess = "true"))
	bool bPropagateJamToPlayerOnSplineEvent = false;

	/** JAM 시 티커테이프 출력 이벤트 발행 여부 (AI에서는 false 권장) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Jam|Ticker", meta = (AllowPrivateAccess = "true"))
	bool bEnableTickerPrintOnJam = true;

	/** JAM 시 티커테이프가 날아가는 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Jam|Ticker", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float TickerEjectDistance = 30.0f;

	/** JAM ShotCount 관련 로그 출력 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Jam|Debug", meta = (AllowPrivateAccess = "true"))
	bool bEnableShotCountLog = true;

	/** BeginPlay 시 JAM 카운트를 자동 시작 (AI 권장) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Jam|AI", meta = (AllowPrivateAccess = "true"))
	bool bAutoStartJamCounting = false;

	/** 자동 시작 시 ShotCount를 0으로 초기화할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Jam|AI", meta = (AllowPrivateAccess = "true"))
	bool bResetShotCountOnAutoStart = true;

	void SetJammedInternal(bool bNewJammed);
	void BroadcastJamSplineEvent();
	void PropagateJamToPlayerWeapon();

	FTimerHandle JamSplineEventTimerHandle;
	TWeakObjectPtr<APawn> CachedPlayerPawn;
	TWeakObjectPtr<UWeaponJamComponent> CachedPlayerWeaponJam;
};
