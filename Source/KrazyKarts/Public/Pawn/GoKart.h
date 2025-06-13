// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GoKart.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_BODY()

	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float TimeStamp;
	
};

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

	void SimulateMove(FGoKartMove Move);

	FVector GetAirResistance();
	FVector GetRollingResistance();
	
	void UpdateLocationFromVelocity(float DeltaTime);
	void ApplyRotation(float InDeltaTime, float InSteeringThrow);

	// Mass of car in Kg.
	UPROPERTY(EditAnywhere)
	float Mass = 1000;

	// Force applied to car when the throttle is fully down (N).
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;

	/*// Number of degrees rotated per second at full control throw (degrees/s).
	UPROPERTY(EditAnywhere)
	float MaxDegreesPerSecond = 90;*/

	// Minimum radius of the car turning circle at full lock (m).
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 2.f;

	// Directly proportional to Drag (Kg/m)
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16;

	// Directly proportional to Rolling Resistance (Kg/t)
	UPROPERTY(EditAnywhere)
	float RollingResistanceCoefficient = 0.02f;

	// seconds to reach max speed
	UPROPERTY(EditAnywhere)
	float TimeToMaxSpeed = 5.f;

	float Throttle;
	float SteeringThrow;
	
	//UPROPERTY(Replicated)
	FVector Velocity;

	/*UPROPERTY(ReplicatedUsing = OnRep_ReplicatedTransform)
	FTransform ReplicatedTransform;*/

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;	
	
	/*UFUNCTION()
	void OnRep_ReplicatedTransform();*/

	UFUNCTION()
	void OnRep_ServerState();

};
