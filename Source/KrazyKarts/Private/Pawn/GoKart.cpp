// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/GoKart.h"

AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	MaxSpeed = MaxDrivingForce / Mass * TimeToMaxSpeed;
	
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Throttle = FMath::FInterpTo(Throttle, TargetThrottle, DeltaTime, ThrottleInterpSpeed);

	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;
	Velocity += Acceleration * DeltaTime;

	/*if (Throttle == 0.f && Velocity.SizeSquared() < 1.0f)
	{
		Velocity = FVector::ZeroVector;
	}*/

	ApplyRotation(DeltaTime);
	UpdateLocationFromVelocity(DeltaTime);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1,                             // Key (unique ID for message)
			0.0f,                          // Duration (0 = this frame only)
			FColor::Green,                // Color
			FString::Printf(TEXT("Velocity: %s (%.1f cm/s)"), *Velocity.ToString(), Velocity.Size())
		);
	}
}

void AGoKart::ApplyRotation(float DeltaTime)
{
	if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return;
	}

	float SpeedRatio = Velocity.Size() / MaxSpeed;
	SpeedRatio = FMath::Clamp(SpeedRatio, 0.0f, 1.0f);

	float RotationAngle = MaxDegreesPerSecond * DeltaTime * SteeringThrow * SpeedRatio;

	FQuat RotationDelta(GetActorUpVector(), FMath::DegreesToRadians(RotationAngle));
	Velocity = RotationDelta.RotateVector(Velocity);
	AddActorWorldRotation(RotationDelta);
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * 100 * DeltaTime;
	FHitResult Hit;
	AddActorWorldOffset(Translation, true, &Hit);

	if (Hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Accelerate);
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
		EnhancedInput->BindAction(SteerAction, ETriggerEvent::Triggered, this, &ThisClass::Steer);
		EnhancedInput->BindAction(SteerAction, ETriggerEvent::Completed, this, &ThisClass::ResetSteering);
	}
}

FVector AGoKart::GetAirResistance()
{
	if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKart::GetRollingResistance()
{
	if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	float NormalForce = Mass * -GetWorld()->GetGravityZ() / 100; // Gravity is already in cm/s²
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

void AGoKart::Accelerate(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();

	// Clamp to forward/backward intent
	TargetThrottle = FMath::Clamp(InputValue, -1.0f, 1.0f);
}

void AGoKart::Steer(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	SteeringThrow = InputValue;
}

void AGoKart::ResetSteering(const FInputActionValue& Value)
{
	SteeringThrow = 0.0f;
}

void AGoKart::Look(const FInputActionValue& Value)
{
	FVector2D LookVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
}

