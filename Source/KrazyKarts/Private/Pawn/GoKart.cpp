// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/GoKart.h"

#include "Components/GoKartMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"

AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>("MovementComponent");
	
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

	if (!MovementComponent) return;

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		MovementComponent->SimulateMove(Move);
		UnacknowledgedMoves.Add(Move);
		Server_SendMove(Move);	
	}

	// Server and in control of the pawn
	if (GetLocalRole() == ROLE_Authority && IsLocallyControlled())
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);	
	}

	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}
	
	DrawDebugString(GetWorld(), FVector(0.f,0.f,100.f), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);	
	
}

void AGoKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);
	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
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

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	MovementComponent->SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();		
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{
	return true; // TODO: Better Validation
}

void AGoKart::Accelerate(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	MovementComponent->SetThrottle(InputValue);
}

void AGoKart::ResetAcceleration()
{
	MovementComponent->SetThrottle(0.f);
	Server_ResetAcceleration();
}

void AGoKart::Server_ResetAcceleration_Implementation()
{
	MovementComponent->SetThrottle(0.f);
}

bool AGoKart::Server_ResetAcceleration_Validate()
{
	return true;
}

void AGoKart::Steer(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	MovementComponent->SetSteeringThrow(InputValue);
}

void AGoKart::ResetSteering()
{
	MovementComponent->SetSteeringThrow(0.0f);
	Server_ResetSteering();
}

void AGoKart::Server_ResetSteering_Implementation()
{
	MovementComponent->SetSteeringThrow(0.0f);
}

bool AGoKart::Server_ResetSteering_Validate()
{
	return true;
}

