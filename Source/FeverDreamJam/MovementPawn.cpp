// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementPawn.h"
#include "Components/StaticMeshComponent.h"
// Sets default values
AMovementPawn::AMovementPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	//Root component is the base of the actor, all other components will be attached to it
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	//Mesh Component
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);

	//give control of this pawn to player0 immediately, can be changed later in the editor or through code
	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

// Called when the game starts or when spawned
void AMovementPawn::BeginPlay()
{
	Super::BeginPlay();

}


void AMovementPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//GetClampedToMaxSize is used to prevent faster diagonal movement when both forward and right input are given
	if (!MoveInput.IsNearlyZero()) {
		FVector Movement = MoveInput.GetClampedToMaxSize(1.0f) * MoveSpeed * DeltaTime;
		AddActorWorldOffset(Movement, true);
	}

}

// Called to bind functionality to input
void AMovementPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMovementPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMovementPawn::MoveRight);

}

void AMovementPawn::MoveForward(float Value)
{
	MoveInput.X = Value;
}

void AMovementPawn::MoveRight(float Value)
{
	MoveInput.Y = Value;
}