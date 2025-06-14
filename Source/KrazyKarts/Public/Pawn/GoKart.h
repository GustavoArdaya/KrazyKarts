// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputSubsystems.h"
#include "Components/GoKartMovementComponent.h"
#include "GoKart.generated.h"

class UGoKartMovementReplicator;
class UGoKartMovementComponent;

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	AGoKart();

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	UGoKartMovementComponent* MovementComponent;

	UPROPERTY(VisibleAnywhere, Category = "Replication")
	UGoKartMovementReplicator* MovementReplicator;

protected:
	virtual void BeginPlay() override;
	
	// Input Mapping Context
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	// Input Actions
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* AccelerateAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* SteerAction;	

	UFUNCTION()
	void Accelerate(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ResetAcceleration();

	UFUNCTION()
	void ResetAcceleration();

	UFUNCTION()
	void Steer(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ResetSteering();

	void ResetSteering();

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


};
