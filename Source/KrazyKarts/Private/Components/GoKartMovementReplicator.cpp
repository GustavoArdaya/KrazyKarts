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
	
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}

void UGoKartMovementReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);	

	if (UGoKartMovementComponent* MovementComponent = Cast<AGoKart>(GetOwner())->MovementComponent)
	{
		ENetRole OwnerRole = GetOwner()->GetLocalRole();
		bool OwnerIsLocallyControlled = Cast<AGoKart>(GetOwner())->IsLocallyControlled();
		
		if (OwnerRole == ROLE_AutonomousProxy)
		{
			FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
			MovementComponent->SimulateMove(Move);
			UnacknowledgedMoves.Add(Move);
			Server_SendMove(Move);	
		}

		// Server and in control of the pawn
		if (OwnerRole == ROLE_Authority && OwnerIsLocallyControlled)
		{
			FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
			Server_SendMove(Move);	
		}

		if (OwnerRole == ROLE_SimulatedProxy)
		{
			MovementComponent->SimulateMove(ServerState.LastMove);
		}
	}
}

void UGoKartMovementReplicator::OnRep_ServerState()
{
	AGoKart* Kart = Cast<AGoKart>(GetOwner());
	
	Kart->SetActorTransform(ServerState.Transform);
	Kart->MovementComponent->SetVelocity(ServerState.Velocity);
	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		Kart->MovementComponent->SimulateMove(Move);
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

void UGoKartMovementReplicator::Server_SendMove_Implementation(FGoKartMove Move)
{
	AGoKart* Kart = Cast<AGoKart>(GetOwner());
	Kart->MovementComponent->SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = Kart->GetActorTransform();
	ServerState.Velocity = Kart->MovementComponent->GetVelocity();		
}

bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Move)
{
	return true; // TODO: Better Validation
}

