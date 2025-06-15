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
		ClientTick(DeltaTime);
	}
}

void UGoKartMovementReplicator::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
		case ROLE_AutonomousProxy:
			AutonomousProxy_OnRep_ServerState();
			break;
		case ROLE_SimulatedProxy:
			SimulatedProxy_OnRep_ServerState();
			break;
		default:
			break;		
	}
}

void UGoKartMovementReplicator::AutonomousProxy_OnRep_ServerState()
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

void UGoKartMovementReplicator::SimulatedProxy_OnRep_ServerState()
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
	
	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;
	ClientStartTransform = GetOwner()->GetActorTransform();
	ClientStartVelocity = MovementComponent->GetVelocity();
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

void UGoKartMovementReplicator::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;
	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER || !MovementComponent) return;

	float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;
	FVector TargetLocation = ServerState.Transform.GetLocation();
	FVector StartLocation = ClientStartTransform.GetLocation();
	FVector StartDerivative = ClientStartVelocity;
	float VelocityToDerivative = ClientTimeBetweenLastUpdates * 100;
	FVector TargetDerivative = ServerState.Velocity * VelocityToDerivative; // Vel is m/s but loc is in cm
	
	FVector NewLocation = FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	GetOwner()->SetActorLocation(NewLocation);

	FVector NewDerivative = FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative;
	MovementComponent->SetVelocity(NewVelocity);
	
	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();

	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);
	GetOwner()->SetActorRotation(NewRotation);
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

