// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Sequence/TickerPaperTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TheseusEventBusSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTheseusEventBus, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTickerPrintRequested, const FTickerPrintRequest&, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnForceStopAiming);

UCLASS()
class DEV_THESEUS_API UTheseusEventBusSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Theseus|EventBus")
    void PublishTickerPrintRequest(const FTickerPrintRequest& Request);

    UPROPERTY(BlueprintAssignable, Category = "Theseus|EventBus")
    FOnTickerPrintRequested OnTickerPrintRequested;

    // 조준 강제 해제 신호. 매니저가 인쇄 몽타주 직전에 Broadcast하고,
    // 플레이어 캐릭터(BP_Theseus_PCH_Player)가 구독해 Trigger Stop Aim을 호출한다.
    UPROPERTY(BlueprintAssignable, Category = "Theseus|EventBus")
    FOnForceStopAiming OnForceStopAiming;
};
