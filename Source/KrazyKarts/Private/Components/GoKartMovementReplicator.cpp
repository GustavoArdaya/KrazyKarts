// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/GoKartMovementReplicator.h"
#include "Net/UnrealNetwork.h"
#include "Pawn/GoKart.h"

UGoKartMovementReplicator::UGoKartMovementReplicator()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);
}

void UGoKartMovementReplicator::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
	
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}

void UGoKartMovementReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);	

	if (!MovementComponent) return;

	FGoKartMove LastMove = MovementComponent->GetLastMove();
	
	bool bIsLocallyControlled = Cast<APawn>(GetOwner())->IsLocallyControlled();
	
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		
		UnacknowledgedMoves.Add(MovementComponent->GetLastMove());
		Server_SendMove(LastMove);	
	}
	// Server and in control of the pawn
	if (GetOwnerRole() == ROLE_Authority && bIsLocallyControlled)
	{
		UpdateServerState(LastMove);
		
	}
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}
}

void UGoKartMovementReplicator::OnRep_ServerState()
{
	if (!MovementComponent)
	{
		MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
		if (!MovementComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("MovementComponent is null in OnRep_ServerState"));
			return;
		}
	}

	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);
	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

void UGoKartMovementReplicator::ClearAcknowledgedMoves(FGoKartMove LastMove)
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

void UGoKartMovementReplicator::UpdateServerState(const FGoKartMove& Move)
{
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();		
	ServerState.Velocity = MovementComponent->GetVelocity();	
}

void UGoKartMovementReplicator::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (!MovementComponent) return;
	
	MovementComponent->SimulateMove(Move);
	UpdateServerState(Move);
}

bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Move)
{
	return true; // TODO: Better Validation
}

