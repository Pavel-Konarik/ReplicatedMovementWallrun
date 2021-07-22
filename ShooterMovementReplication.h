// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShooterMovementTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <GameFramework/CharacterMovementReplication.h>


class FSavedMove_ShooterCharacter : public FSavedMove_Character
{
public:

	typedef FSavedMove_Character Super;

	// Settings
	float WallNormalThresholdCombine = 0.01;

	// Gameplay variables
	// Done
	uint8 bWallrunWantsToUnstick : 1;
	// Done
	float WantsToUnstickTimeRemaining = 0.0f;
	// Done
	EWallRunSide WallRunSide = EWallRunSide::Left;
	// Done, possible need to decrease the WallNormalThresholdCombine
	FVector WallRunWallNormal = FVector::ZeroVector;
	// Done
	float CurrentWallRunEndGravity = 1.0f;
	// Done
	EWallRunState WallRunState = EWallRunState::End;
	// Done
	float WallRunTimeRemaining = 0.0f;
	// Done
	float WallRunCooldownLeftTimeRemaining = 0.0f;
	// Done
	float WallRunCooldownRightTimeRemaining = 0.0f;

	// Resets all saved variables.
	virtual void Clear() override;
	// Store input commands in the compressed flags.
	virtual uint8 GetCompressedFlags() const override;
	// This is used to check whether or not two moves can be combined into one.
	// Basically you just check to make sure that the saved variables are the same.
	virtual bool CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* Character, float MaxDelta) const override;

	virtual void CombineWith(const FSavedMove_Character* OldMove, ACharacter* InCharacter, APlayerController* PC, const FVector& OldStartLocation) override;
	// Sets up the move before sending it to the server. 
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
	// Sets variables on character movement component before making a predictive correction.
	virtual void PrepMoveFor(class ACharacter* Character) override;


	/** Returns true if this move is an "important" move that should be sent again if not acked by the server */
	virtual bool IsImportantMove(const FSavedMovePtr& LastAckedMove) const;


};

class FNetworkPredictionData_Client_ShooterCharacter : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	// Constructor
	FNetworkPredictionData_Client_ShooterCharacter(const UCharacterMovementComponent& ClientMovement);

	//brief Allocates a new copy of our custom saved move
	virtual FSavedMovePtr AllocateNewMove() override;
};


struct SHOOTERGAME_API FShooterCharacterNetworkMoveData : FCharacterNetworkMoveData
{
public:
	bool bWantsToUnstick = false;

	/**
	 * Given a FSavedMove_Character from UCharacterMovementComponent, fill in
	 * data in this struct with relevant movement data. Note that the instance
	 * of the FSavedMove_Character is likely a custom struct of a derived struct
	 * of your own, if you have added your own saved move data.
	 * @see UCharacterMovementComponent::AllocateNewMove()
	 */
	virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;

	/**
	 * Serialize the data in this struct to or from the given FArchive. This
	 * packs or unpacks the data in to a variable-sized data stream that is sent
	 * over the network from client to server.
	 * @see UCharacterMovementComponent::CallServerMovePacked
	 */
	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;

private:
	typedef FCharacterNetworkMoveData Super;
};


struct SHOOTERGAME_API FShooterCharacterNetworkMoveDataContainer
	: FCharacterNetworkMoveDataContainer
{
public:
	FShooterCharacterNetworkMoveDataContainer();

private:
	typedef FCharacterNetworkMoveDataContainer Super;
	FShooterCharacterNetworkMoveData BFDefaultMoveData[3];
};