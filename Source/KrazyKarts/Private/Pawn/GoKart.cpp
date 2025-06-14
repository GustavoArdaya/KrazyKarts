// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/GoKart.h"

#include "Components/GoKartMovementComponent.h"
#include "Components/GoKartMovementReplicator.h"
#include "Net/UnrealNetwork.h"

AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>("MovementComponent");
	MovementReplicator = CreateDefaultSubobject<UGoKartMovementReplicator>("MovementReplicator");
	
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

/*void AGoKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	
}*/

void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);	
	
	DrawDebugString(GetWorld(), FVector(0.f,0.f,100.f), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);	
	
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

