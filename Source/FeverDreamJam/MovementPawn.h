// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"

class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
class UCameraComponent;
#include "MovementPawn.generated.h"		


UCLASS()
class FEVERDREAMJAM_API AMovementPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AMovementPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void CheckGrounded();
	void Jump();
	void StopJumping();
	bool isGrounded = false;
	UPROPERTY(EditAnywhere)
	float MoveSpeed = 600.0f;

	UPROPERTY(EditAnywhere)
	float Acceleration = 3000.0f;

	UPROPERTY(EditAnywhere)
	float GroundFriction = 8.0f;

	UPROPERTY(EditAnywhere)
	float Gravity = -980.0f;

	UPROPERTY(EditAnywhere)
	float GroundCheckDistance = 150.0f;

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MouseSensitivity = 1.0f;
	USceneComponent* Root;
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere)
	FVector MoveInput;
	// Called to bind functionality to input


private:
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

};
