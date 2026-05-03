// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementPawn.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"



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

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Pawn BeginPlay"));
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No PlayerController"));
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());

	if (Subsystem && InputMappingContext)
	{
		Subsystem->AddMappingContext(InputMappingContext, 0);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Added IMC"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Missing IMC or Subsystem"));
	}
}

void AMovementPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//GetClampedToMaxSize is used to prevent faster diagonal movement when both forward and right input are given
	if (!MoveInput.IsNearlyZero()) {
		FVector Movement = MoveInput.GetClampedToMaxSize(1.0f) * MoveSpeed * DeltaTime;
		AddActorWorldOffset(Movement, true);
	}
	CheckGrounded();
}

// Called to bind functionality to input
void AMovementPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (EIC) {
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMovementPawn::Move);
		EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMovementPawn::Move);
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AMovementPawn::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMovementPawn::StopJumping);
	}

}

void AMovementPawn::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1,
			0.f,
			FColor::Green,
			Value.ToString()
		);
	}

	MoveInput.X = MovementVector.Y; // W/S
	MoveInput.Y = MovementVector.X; // A/D
}

void AMovementPawn::Look(const FInputActionValue& Value)
{
	FVector2D LookVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
}

void AMovementPawn::CheckGrounded()
{
	//FVector trace to check if the pawn is grounded, starts at the pawn location and goes down by GroundCheckDistance units, if it hits something the pawn is grounded 
	//We currently draw this vector every tick, this seems expensive but unreal creates a bunch of traces per tick anyways so it doesn't seem to cause any performance issues
	FVector Start = GetActorLocation();
	FVector End = Start - FVector(0, 0, GroundCheckDistance);
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	// this line performs the line trace and sets isGrounded to true if it hits something, false otherwise
	isGrounded = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	DrawDebugLine(
		GetWorld(),
		Start,
		End,
		isGrounded ? FColor::Green : FColor::Red,
		false,
		0.0f,
		0,
		2.0f
	);

}

void AMovementPawn::Jump()
{
	if (isGrounded) {
		//Add an impulse upwards to the mesh component, this will cause the pawn to jump
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Grounded, Should be Jumping!")));
		}
		Mesh->AddImpulse(FVector(0, 0, 500.0f), NAME_None, true);
	}

}

void AMovementPawn::StopJumping()
{ 
	//Currently does nothing, but could be used to implement variable jump height by reducing the upward velocity when the jump button is released
}