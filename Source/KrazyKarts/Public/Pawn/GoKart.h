// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/GoKartMovementComponent.h"
#include "GoKart.generated.h"

class UGoKartMovementComponent;

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


UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	AGoKart();

protected:
	virtual void BeginPlay() override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, Category = "Movement")
	UGoKartMovementComponent* MovementComponent;
	
	// Input Mapping Context
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	// Input Actions
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* AccelerateAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* SteerAction;

	/*UFUNCTION(Server, Reliable, WithValidation)
	void Server_Accelerate(float Value);*/

	UFUNCTION(Server, Reliable,WithValidation)
	void Server_SendMove(FGoKartMove Move);

	UFUNCTION()
	void Accelerate(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ResetAcceleration();

	UFUNCTION()
	void ResetAcceleration();

	/*UFUNCTION(Server, Reliable, WithValidation)
	void Server_Steer(float Value);*/

	UFUNCTION()
	void Steer(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ResetSteering();

	void ResetSteering();

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	
	void ClearAcknowledgedMoves(FGoKartMove LastMove);	

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;	

	UFUNCTION()
	void OnRep_ServerState();

	TArray<FGoKartMove> UnacknowledgedMoves;

};
