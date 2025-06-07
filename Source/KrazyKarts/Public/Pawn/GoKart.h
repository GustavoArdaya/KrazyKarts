// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GoKart.generated.h"

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	AGoKart();

protected:
	virtual void BeginPlay() override;
	

	// Input Mapping Context
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	// Input Actions
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* SteerAction;

	UFUNCTION()
	void Accelerate(const FInputActionValue& Value);

	UFUNCTION()
	void Steer(const FInputActionValue& Value);

	UFUNCTION()
	void ResetSteering(const FInputActionValue& Value);

	UFUNCTION()
	void Look(const FInputActionValue& Value);

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	FVector GetResistance();
	void UpdateLocationFromVelocity(float DeltaTime);
	void ApplyRotation(float DeltaTime);

	// Mass of car in Kg.
	UPROPERTY(EditAnywhere)
	float Mass = 1000;

	// Force applied to car when the throttle is fully down (N).
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;

	// Number of degrees rotated per second at full control throw (degrees/s).
	UPROPERTY(EditAnywhere)
	float MaxDegreesPerSecond = 90;

	// Directly proportional to Drag (Kg/m)
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16;

	// seconds to reach max speed
	UPROPERTY(EditAnywhere)
	float TimeToMaxSpeed = 5.f;

	FVector Velocity;
	float Throttle;
	float SteeringThrow;
	float MaxSpeed;

};
