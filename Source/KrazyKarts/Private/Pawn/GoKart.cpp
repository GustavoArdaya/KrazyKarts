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

	if (HasAuthority())
	{
		SetReplicateMovement(true);		
	}
	
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

void AGoKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	/*DOREPLIFETIME(AGoKart, ReplicatedTransform);
	DOREPLIFETIME(AGoKart, Velocity);*/
	DOREPLIFETIME(AGoKart, ServerState);
}

void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		FGoKartMove Move;
		Move.DeltaTime = DeltaTime;
		Move.SteeringThrow = SteeringThrow;
		Move.Throttle = Throttle;
		// Move.TimeStamp // TODO

		Server_SendMove(Move);
		SimulateMove(Move);
	}
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 3, FColor::Cyan, FString::Printf(TEXT("Velocity: %.2f"), Velocity.Size()));
	}	
	
	DrawDebugString(GetWorld(), FVector(0.f,0.f,100.f), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);	
	
}

void AGoKart::ApplyRotation(float InDeltaTime, float InSteeringThrow)
{
	if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return;
	}

	float Speed = Velocity.Size(); // Magnitude of the velocity vector
	float MinSpeed = 0.f;
	float MaxSpeed = 25.f; // Adjust based on your expected top speed

	float MaxSensitivity = 10.f; // High sensitivity at low speed
	float MinSensitivity = 4.f; // Low sensitivity at high speed

	float SteeringSensitivity = FMath::GetMappedRangeValueClamped(
		FVector2D(MinSpeed, MaxSpeed),
		FVector2D(MaxSensitivity, MinSensitivity),
		Speed
	);

	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * InDeltaTime;
	float RotationAngle = (DeltaLocation / MinTurningRadius) * InSteeringThrow * SteeringSensitivity;

	FQuat RotationDelta(GetActorUpVector(), FMath::DegreesToRadians(RotationAngle));
	Velocity = RotationDelta.RotateVector(Velocity);
	AddActorWorldRotation(RotationDelta);
}

void AGoKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;
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
		EnhancedInput->BindAction(AccelerateAction, ETriggerEvent::Triggered, this, &ThisClass::Accelerate);
		EnhancedInput->BindAction(AccelerateAction, ETriggerEvent::Completed, this, &ThisClass::ResetAcceleration);
		EnhancedInput->BindAction(SteerAction, ETriggerEvent::Triggered, this, &ThisClass::Steer);
		EnhancedInput->BindAction(SteerAction, ETriggerEvent::Completed, this, &ThisClass::ResetSteering);
	}
}

void AGoKart::SimulateMove(FGoKartMove Move)
{
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Acceleration = Force / Mass;
	Velocity += Acceleration * Move.DeltaTime;
	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);
	UpdateLocationFromVelocity(Move.DeltaTime);	
}

FVector AGoKart::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKart::GetRollingResistance()
{
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	float NormalForce = Mass * AccelerationDueToGravity; // Gravity is already in cm/s²
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = Velocity;		
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{
	return true; // TODO: Better Validation
}

void AGoKart::Accelerate(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	Throttle = InputValue;
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
	return true;
}

void AGoKart::Steer(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	SteeringThrow = InputValue;
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
	return true;
}

