// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TickerPaperTypes.generated.h"

USTRUCT(BlueprintType)
struct DEV_THESEUS_API FTickerPrintRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ticker")
    FText Message;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ticker")
    FGameplayTag ReasonTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ticker", meta = (ClampMin = "0.0"))
    float EjectDistance = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ticker", meta = (ClampMin = "0.0"))
    float LifeTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ticker")
    TObjectPtr<AActor> InstigatorActor = nullptr;
};