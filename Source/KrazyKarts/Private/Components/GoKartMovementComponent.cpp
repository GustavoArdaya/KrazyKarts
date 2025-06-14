// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/GoKartMovementComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Pawn/GoKart.h"

UGoKartMovementComponent::UGoKartMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();	
}

void UGoKartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UGoKartMovementComponent::ApplyRotation(float InDeltaTime, float InSteeringThrow)
{
	if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return;
	}	

	float SteeringSensitivity = GetSteeringSensitivity();

	float DeltaLocation = FVector::DotProduct(GetOwner()->GetActorForwardVector(), Velocity) * InDeltaTime;
	float RotationAngle = (DeltaLocation / MinTurningRadius) * InSteeringThrow * SteeringSensitivity;

	FQuat RotationDelta(GetOwner()->GetActorUpVector(), FMath::DegreesToRadians(RotationAngle));
	Velocity = RotationDelta.RotateVector(Velocity);
	GetOwner()->AddActorWorldRotation(RotationDelta);
}

void UGoKartMovementComponent::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * 100 * DeltaTime;
	FHitResult Hit;
	GetOwner()->AddActorWorldOffset(Translation, true, &Hit);

	if (Hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

void UGoKartMovementComponent::SimulateMove(const FGoKartMove& Move)
{
	FVector Force = GetOwner()->GetActorForwardVector() * MaxDrivingForce * Move.Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Acceleration = Force / Mass;
	Velocity += Acceleration * Move.DeltaTime;
	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);
	UpdateLocationFromVelocity(Move.DeltaTime);	
}


FGoKartMove UGoKartMovementComponent::CreateMove(float DeltaTime)
{
	FGoKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;

	AGameStateBase* GameState = GetWorld()->GetGameState<AGameStateBase>();
	if (GameState)
	{
		Move.TimeStamp = GameState->GetServerWorldTimeSeconds();
	}
	else
	{
		Move.TimeStamp = GetWorld()->TimeSeconds;
	}

	return Move;
}

float UGoKartMovementComponent::GetSteeringSensitivity()
{
	const float Gravity = FMath::Abs(GetWorld()->GetGravityZ());
	float RollingResistance = RollingResistanceCoefficient * Mass * Gravity;
	float MaxSpeedEstimate = FMath::Sqrt((MaxDrivingForce - RollingResistance) / DragCoefficient); // m/s
	float Speed = Velocity.Size();
	MaxSpeedEstimate = FMath::Max(MaxSpeedEstimate, 1.f);

	return FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, MaxSpeedEstimate),
		FVector2D(MaxSteeringSensitivity, MinSteeringSensitivity),
		Speed
	);
}

FVector UGoKartMovementComponent::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector UGoKartMovementComponent::GetRollingResistance()
{
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	float NormalForce = Mass * AccelerationDueToGravity; // Gravity is already in cm/s²
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

