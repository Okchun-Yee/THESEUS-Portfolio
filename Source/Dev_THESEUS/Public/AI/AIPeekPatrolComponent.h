// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "AIPeekPatrolComponent.generated.h"

class AActor;
class AAIController;

UENUM(BlueprintType)
enum class EAIPeekPatrolState : uint8
{
	None,
	MoveLeft,
	WaitLeft,
	MoveRight,
	WaitRight,
	Rush
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIPeekRush);

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API UAIPeekPatrolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAIPeekPatrolComponent();

	UFUNCTION(BlueprintCallable, Category="AI|Peek")
	void ConfigurePeekPatrol(AActor* InLeftPoint, AActor* InRightPoint, float InWaitTime, float InTimeoutToRush);

	UFUNCTION(BlueprintCallable, Category="AI|Peek")
	bool StartPeekPatrol();

	UFUNCTION(BlueprintCallable, Category="AI|Peek")
	void StopPeekPatrol(bool bForceRush = false);

	UFUNCTION(BlueprintCallable, Category="AI|Peek")
	bool IsPeekPatrolActive() const { return bPeekActive; }

	UFUNCTION(BlueprintCallable, Category="AI|Peek")
	EAIPeekPatrolState GetCurrentState() const { return CurrentState; }

	UPROPERTY(BlueprintAssignable, Category="AI|Peek")
	FOnAIPeekRush OnPeekRush;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void UpdatePeekPatrol();
	bool MoveToPoint(AActor* TargetPoint);
	void EnterState(EAIPeekPatrolState NewState);
	void EnterRush(const TCHAR* Reason);
	AAIController* ResolveAIController() const;
	void ApplyRushBlackboardTarget(AAIController* AIController) const;

private:
	UPROPERTY(EditAnywhere, Category="AI|Peek", meta=(ClampMin="10.0"))
	float MoveAcceptanceRadius = 120.0f;

	UPROPERTY(EditAnywhere, Category="AI|Peek", meta=(ClampMin="0.05"))
	float UpdateInterval = 0.2f;

	UPROPERTY(EditAnywhere, Category="AI|Peek", meta=(ClampMin="0.0"))
	float PeekWaitTime = 1.2f;

	UPROPERTY(EditAnywhere, Category="AI|Peek", meta=(ClampMin="0.0"))
	float PeekTimeoutToRush = 8.0f;

	UPROPERTY(EditAnywhere, Category="AI|Peek|Debug")
	bool bEnablePeekLog = true;

	UPROPERTY()
	TObjectPtr<AActor> PeekLeftPoint = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> PeekRightPoint = nullptr;

	UPROPERTY(VisibleAnywhere, Category="AI|Peek")
	EAIPeekPatrolState CurrentState = EAIPeekPatrolState::None;

	UPROPERTY(VisibleAnywhere, Category="AI|Peek")
	bool bPeekActive = false;

	UPROPERTY(VisibleAnywhere, Category="AI|Peek")
	float ElapsedTotal = 0.0f;

	UPROPERTY(VisibleAnywhere, Category="AI|Peek")
	float ElapsedInState = 0.0f;

	FTimerHandle PeekUpdateTimerHandle;
};
