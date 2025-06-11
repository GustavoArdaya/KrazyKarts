// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/GoKart.h"

#include "Net/UnrealNetwork.h"

AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
	SetReplicateMovement(true);
	
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

FString GetEnumText(ENetRole Role)
{
	switch (Role)
	{
	case ROLE_Authority:
		return "Authority"; 
	case ROLE_None:
		return "None"; 
	case ROLE_SimulatedProxy:
		return "Simulated Proxy"; 
	case ROLE_AutonomousProxy:
		return "Autonomous Proxy"; 
	case ROLE_MAX:
		return "Max";
	default:
		return "Unknown";
	}
}

/*void AGoKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoKart, Velocity);
	DOREPLIFETIME(AGoKart, Throttle);
	DOREPLIFETIME(AGoKart, SteeringThrow);
}*/

void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1,                             // Key (unique ID for message)
			0.0f,                          // Duration (0 = this frame only)
			FColor::Green,                // Color
			FString::Printf(TEXT("Velocity: %s (%.1f cm/s)"), *Velocity.ToString(), Velocity.Size())
		);
		GEngine->AddOnScreenDebugMessage(
			2,                             // Key (unique ID for message)
			0.0f,                          // Duration (0 = this frame only)
			FColor::Green,                // Color
			FString::Printf(TEXT("SteeringThrow: %.1f"), SteeringThrow)
		);
	}*/
	DrawDebugString(GetWorld(), FVector(0.f,0.f,100.f), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);
	
}

void AGoKart::ApplyRotation(float DeltaTime)
{
	/*if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return;
	}*/
	float SteeringSensibility = 5.f;
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle =  (DeltaLocation / MinTurningRadius) * SteeringThrow * SteeringSensibility;

	FQuat RotationDelta(GetActorUpVector(), FMath::DegreesToRadians(RotationAngle));
	Velocity = RotationDelta.RotateVector(Velocity);
	AddActorWorldRotation(RotationDelta);
}

/*void AGoKart::OnRep_Throttle()
{
}*/

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
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::ResetAcceleration);
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
		EnhancedInput->BindAction(SteerAction, ETriggerEvent::Triggered, this, &ThisClass::Steer);
		EnhancedInput->BindAction(SteerAction, ETriggerEvent::Completed, this, &ThisClass::ResetSteering);
	}
}

FVector AGoKart::GetAirResistance()
{
	/*if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}*/

	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKart::GetRollingResistance()
{
	/*if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}*/

	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	float NormalForce = Mass * AccelerationDueToGravity; // Gravity is already in cm/s²
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

void AGoKart::Accelerate(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	Throttle = InputValue;
	Server_Accelerate(Value);
}

void AGoKart::Server_Accelerate_Implementation(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	Throttle = InputValue;
}

bool AGoKart::Server_Accelerate_Validate(const FInputActionValue& Value)
{
	return FMath::Abs(Value.Get<float>()) <= 1.f;
}

void AGoKart::ResetAcceleration()
{
	Throttle = 0.f;
	Server_ResetAcceleration();
}

void AGoKart::Server_ResetAcceleration_Implementation()
{
	Throttle = 0.f;
}

bool AGoKart::Server_ResetAcceleration_Validate()
{
	return Throttle == 0.f;
}

void AGoKart::Steer(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	SteeringThrow = InputValue;
	Server_Steer(Value);
}


void AGoKart::Server_Steer_Implementation(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	SteeringThrow = InputValue;
}

bool AGoKart::Server_Steer_Validate(const FInputActionValue& Value)
{
	return FMath::Abs(Value.Get<float>()) <= 1.f;
}

void AGoKart::ResetSteering()
{
	SteeringThrow = 0.0f;
	Server_ResetSteering();
}

void AGoKart::Server_ResetSteering_Implementation()
{
	SteeringThrow = 0.0f;
}

bool AGoKart::Server_ResetSteering_Validate()
{
	return SteeringThrow == 0.0f;
}

void AGoKart::Look(const FInputActionValue& Value)
{
	FVector2D LookVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
}

