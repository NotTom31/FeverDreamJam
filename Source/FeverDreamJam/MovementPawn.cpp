// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementPawn.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AMovementPawn::AMovementPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	//Root component is the base of the actor, all other components will be attached to it
	CapsuleCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleCollider"));
	CapsuleCollider->InitCapsuleSize(40.f, 90.f);
	CapsuleCollider->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	SetRootComponent(CapsuleCollider);

	//Mesh Component
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(CapsuleCollider);
	Mesh->SetMobility(EComponentMobility::Movable);

	//Camera Component
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CapsuleCollider);
	Camera->SetRelativeLocation(FVector(0, 0.0f, 64.0f));
	Camera->bUsePawnControlRotation = true;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = true;
	bUseControllerRotationRoll = false;

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
	FVector DesiredDirection = MoveInput.GetClampedToMaxSize(1.0f);

	if (!MoveInput.IsNearlyZero()) {

		Velocity += DesiredDirection * Acceleration * DeltaTime;
		Velocity = Velocity.GetClampedToMaxSize(MaxSpeed);
		
		FVector HorizontalVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);
		HorizontalVelocity = HorizontalVelocity.GetClampedToMaxSize(MaxSpeed);
		Velocity.X = HorizontalVelocity.X;
		Velocity.Y = HorizontalVelocity.Y;
	}
	else {
		//Apply ground friction when no input is given, this will cause the pawn to come to a stop when the player lets go of the movement input
		Velocity.X = FMath::FInterpTo(Velocity.X, 0.0f, DeltaTime, GroundFriction);
		Velocity.Y = FMath::FInterpTo(Velocity.Y, 0.0f, DeltaTime, GroundFriction);
	}
	
	float TargetHalfHeight = isCrouching ? CrouchCapsuleHalfHeight : StandingCapsuleHalfHeight;
	float NewHalfHeight = FMath::FInterpTo(
		CapsuleCollider->GetUnscaledCapsuleHalfHeight(), 
		TargetHalfHeight, 
		DeltaTime, 
		CrouchInterpSpeed);
	CapsuleCollider->SetCapsuleHalfHeight(NewHalfHeight, true);

	float TargetCameraHeight = isCrouching ? CrouchingCameraHeight : StandingCameraHeight;
	FVector CameraLocation = Camera->GetRelativeLocation();
	CameraLocation.Z = FMath::FInterpTo(CameraLocation.Z, TargetCameraHeight, DeltaTime, CrouchInterpSpeed);
	Camera->SetRelativeLocation(CameraLocation);
	CheckGrounded();
	ApplyGravity(DeltaTime);
	FVector Movement = Velocity * DeltaTime;
	MoveWithCollisions(Movement);
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
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMovementPawn::Look);
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AMovementPawn::Interact);
		EIC->BindAction(InteractAction, ETriggerEvent::Completed, this, &AMovementPawn::StopInteract);
		EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &AMovementPawn::StartSprinting);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMovementPawn::StopSprinting);
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &AMovementPawn::StartCrouching);
		EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AMovementPawn::StopCrouching);
	}
}

void AMovementPawn::StartSprinting()
{
	if(!isCrouching) {
		isSprinting = true;
		RefreshMovementState();
	}
}

void AMovementPawn::StopSprinting()
{
	isSprinting = false;
	RefreshMovementState();
}

bool AMovementPawn::CanStandUp() const {
	FVector Start = GetActorLocation();
	FVector End = Start + FVector(0, 0, StandingCapsuleHalfHeight - CrouchCapsuleHalfHeight);
	FCollisionShape StandingCapsule = FCollisionShape::MakeCapsule(
		CapsuleCollider->GetScaledCapsuleRadius(),
		StandingCapsuleHalfHeight
	);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	// If the line trace hits something, we can't stand up
	return !GetWorld()->SweepTestByChannel(
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		StandingCapsule,
		Params
	);
}


void AMovementPawn::StartCrouching()
{
	if (!isSprinting) {
		isCrouching = true;
		RefreshMovementState();
	}
}

void AMovementPawn::StopCrouching()
{
	if (!CanStandUp()) {
		return;
	}

	isCrouching = false;
	RefreshMovementState();
}

void AMovementPawn::RefreshMovementState() {
	
	if (isCrouching) {
		MaxSpeed = CrouchSpeed;
	}
	else if (isSprinting) {
		MaxSpeed = SprintSpeed;
	}
	else {
		MaxSpeed = WalkSpeed;
	}
}

void AMovementPawn::Move(const FInputActionValue& Value)
{	
	FVector2D Input = Value.Get<FVector2D>();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1,
			0.f,	
			FColor::Green,
			Value.ToString()
		);
	}

	if (!Controller) {
		return;
	}

	FRotator ControlRotation = Controller->GetControlRotation();
	FRotator YawRotation(0, ControlRotation.Yaw, 0);
	FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	MoveInput = ForwardDirection * Input.Y + RightDirection * Input.X;
}


void AMovementPawn::MoveWithCollisions(const FVector& DesiredMovement)
{
	if (!CapsuleCollider || DesiredMovement.IsNearlyZero()) {
		return;
	}

	FVector RemainingMovement = DesiredMovement;
	const int32 MaxIterations = 5;

	for (int i = 0; i < MaxIterations; i++)
	{
		if (RemainingMovement.IsNearlyZero()) {
			break;
		}
		
		FVector Start = GetActorLocation();
		FVector End = Start + RemainingMovement;

		FHitResult Hit;
		float CapsuleRadius = CapsuleCollider->GetScaledCapsuleRadius();
		float CapsuleHalfHeight = CapsuleCollider->GetScaledCapsuleHalfHeight();

		FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);

		FCollisionShape CapsuleShape2D = FCollisionShape::MakeCapsule(
			CapsuleRadius, 
			CapsuleHalfHeight
		);
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		bool bHit = GetWorld()->SweepSingleByChannel(
			Hit,
			Start,
			End,
			FQuat::Identity,
			ECC_Pawn,
			CapsuleShape,
			Params
		);

		if (!bHit) {
			SetActorLocation(End);
			break;
		}

		//Move only up to the safe part before impact
		FVector SafeMove = RemainingMovement * Hit.Time;
		SetActorLocation(Start + SafeMove);


		//Push slightlyaway from the surface to prevent sticking like velcro
		FVector Depenteration = Hit.Normal * 1.0f;
		AddActorWorldOffset(Depenteration, false);
		FVector UsedMovement = SafeMove;
		FVector LeftoverMovement = RemainingMovement - UsedMovement;

		RemainingMovement = FVector::VectorPlaneProject(LeftoverMovement, Hit.Normal);
		Velocity = FVector::VectorPlaneProject(Velocity, Hit.Normal);

		RemainingMovement *= 0.9f; // reduce remaining movement to prevent getting stuck in corners
	}
}

void AMovementPawn::Look(const FInputActionValue& Value)
{
	FVector2D LookVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookVector.X * MouseSensitivity);
	AddControllerPitchInput(LookVector.Y * MouseSensitivity);
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

void AMovementPawn::CheckInteractable()
{
	if (!Camera) return;

	FVector Start = Camera->GetComponentLocation();
	FVector Forward = Camera->GetForwardVector();
	FVector End = Start + (Forward * InteractableCheckDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	DrawDebugLine(
		GetWorld(),
		Start,
		End,
		bHit ? FColor::Green : FColor::Red,
		false,
		0.0f,
		0,
		2.0f
	);

	if (bHit)
	{
		// Debug what you hit
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.f,
				FColor::Yellow,
				FString::Printf(TEXT("Hit: %s"), *Hit.GetActor()->GetName())
			);
		}
	}
}

void AMovementPawn::ApplyGravity(float DeltaTime)
{
	if (!isGrounded) {
		Velocity.Z += GetWorld()->GetGravityZ() * DeltaTime;
	}
	else if (Velocity.Z < 0) {
		Velocity.Z = 0;
	}
}

void AMovementPawn::Jump()
{
	if (isGrounded) {
		Velocity.Z = JumpStrength;
	}
}

void AMovementPawn::StopJumping()
{ 
	//Currently does nothing, but could be used to implement variable jump height by reducing the upward velocity when the jump button is released
}

void AMovementPawn::Interact()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Checking for interactable!")));
	}
	CheckInteractable();
}

void AMovementPawn::StopInteract()
{
	 
}