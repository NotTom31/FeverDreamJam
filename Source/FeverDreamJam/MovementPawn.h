// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
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

	// Called to bind functionality to input
private:
	UPROPERTY(VisibleAnywhere)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	FVector MoveInput;
	void MoveForward(float Value);
	void MoveRight(float Value);
	void CheckGrounded();
	void Jump();


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
	float GroundCheckDistance = 100.0f;

	
};
