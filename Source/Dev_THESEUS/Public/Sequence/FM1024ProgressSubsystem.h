// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FM1024ProgressSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFM1024Progress, Log, All);

UCLASS()
class DEV_THESEUS_API UFM1024ProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Theseus|FM1024")
	void SetFM1024Acquired(bool bAcquired);

	UFUNCTION(BlueprintPure, Category = "Theseus|FM1024")
	bool IsFM1024Acquired() const;

	UFUNCTION(BlueprintCallable, Category = "Theseus|FM1024")
	void ResetFM1024Progress();

private:
	UPROPERTY()
	bool bFM1024Acquired = false;
};
