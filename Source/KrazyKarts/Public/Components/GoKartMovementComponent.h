// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"

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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGoKartMovementComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void SimulateMove(const FGoKartMove& Move);
	FGoKartMove CreateMove(float DeltaTime);

	FORCEINLINE FVector GetVelocity() const { return Velocity; }
	FORCEINLINE float GetThrottle() const { return Throttle; }
	FORCEINLINE float GetSteeringThrow() const { return SteeringThrow; }

	FORCEINLINE void SetVelocity(const FVector& NewVelocity) { Velocity = NewVelocity; }
	FORCEINLINE void SetThrottle(float NewThrottle) { Throttle = NewThrottle; }
	FORCEINLINE void SetSteeringThrow(float NewThrow) { SteeringThrow = NewThrow; }

private:


	float GetSteeringSensitivity();
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

	// Minimum radius of the car turning circle at full lock (m).
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 2.f;

	UPROPERTY(EditAnywhere)
	float MinSteeringSensitivity = 4.f;

	UPROPERTY(EditAnywhere)
	float MaxSteeringSensitivity = 10.f;

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
	FVector Velocity;
};
