// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/GoKart.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"

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
	
	DOREPLIFETIME(AGoKart, ServerState);
}

void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		FGoKartMove Move = CreateMove(DeltaTime);
		if (!HasAuthority())
		{
			UnacknowledgedMoves.Add(Move);
		}

		Server_SendMove(Move);
		SimulateMove(Move);
	}
	
	if (GEngine)
	{
		const FString RoleText = GetEnumText(GetLocalRole());
		const FString VelocityText = FString::Printf(TEXT("Velocity (Role: %s): %.2f"), *RoleText, Velocity.Size());
		GEngine->AddOnScreenDebugMessage(1, 3, FColor::Cyan, VelocityText);
	}
	
	DrawDebugString(GetWorld(), FVector(0.f,0.f,100.f), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);	
	
}

void AGoKart::ApplyRotation(float InDeltaTime, float InSteeringThrow)
{
	if (Velocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return;
	}	

	float SteeringSensitivity = GetSteeringSensitivity();

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
	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		SimulateMove(Move);
	}
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

void AGoKart::SimulateMove(const FGoKartMove& Move)
{
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Acceleration = Force / Mass;
	Velocity += Acceleration * Move.DeltaTime;
	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);
	UpdateLocationFromVelocity(Move.DeltaTime);	
}

FGoKartMove AGoKart::CreateMove(float DeltaTime)
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

void AGoKart::ClearAcknowledgedMoves(FGoKartMove LastMove)
{
	TArray<FGoKartMove> NewMoves;
	
	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if (Move.TimeStamp >= LastMove.TimeStamp)
		{
			NewMoves.Add(Move);
		}
	}	
	UnacknowledgedMoves = NewMoves;
}

float AGoKart::GetSteeringSensitivity()
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

