// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GoKartMovementComponent.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementReplicator.generated.h"

USTRUCT()
struct FGoKartState
{
	GENERATED_BODY()

	UPROPERTY()
	FTransform Transform;
	
	UPROPERTY()
	FVector Velocity;
	
	UPROPERTY()
	FGoKartMove LastMove;	
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementReplicator : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGoKartMovementReplicator();

protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(Server, Reliable,WithValidation)
	void Server_SendMove(FGoKartMove Move);
	
private:

	void ClearAcknowledgedMoves(FGoKartMove LastMove);
	void UpdateServerState(const FGoKartMove& Move);
	void ClientTick(float DeltaTime);
	
	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;	

	UFUNCTION()
	void OnRep_ServerState();
	void AutonomousProxy_OnRep_ServerState();
	void SimulatedProxy_OnRep_ServerState();

	TArray<FGoKartMove> UnacknowledgedMoves;

	float ClientTimeSinceUpdate;
	float ClientTimeBetweenLastUpdates;
	FTransform ClientStartTransform;
	FVector ClientStartVelocity;

	UPROPERTY()
	UGoKartMovementComponent* MovementComponent;
		
};
