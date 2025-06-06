// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/GoKart.h"

AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
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

	FVector Translation = Velocity * 100 * DeltaTime;
	AddActorWorldOffset(Translation);

}

void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::MoveForward);
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	}
}

void AGoKart::MoveForward(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>(); // Get axis input, e.g., -1.0 to 1.0

	// Optionally clamp or filter input value
	InputValue = FMath::Clamp(InputValue, -1.0f, 1.0f);

	// Update velocity in the forward direction
	Velocity = GetActorForwardVector() * InputValue * 20.f;
}

void AGoKart::Look(const FInputActionValue& Value)
{
	FVector2D LookVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
}

