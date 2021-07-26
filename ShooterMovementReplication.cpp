// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterMovementReplication.h"
#include "ShooterCharacterMovement.h"
#include <GameFramework/Character.h>
#include "ShooterCharacter.h"


void FSavedMove_ShooterCharacter::Clear()
{
	Super::Clear();

	// Clear all values
	bWallrunWantsToUnstick = 0;
	WantsToUnstickTimeRemaining = 0.0f;
	WallRunSide = EWallRunSide::Left;
	WallRunWallNormal = FVector::ZeroVector;
	CurrentWallRunEndGravity = 1.0f;
	WallRunState = EWallRunState::End;

	WallRunTimeRemaining = 0.0f;
	WallRunCooldownLeftTimeRemaining = 0.0f;
	WallRunCooldownRightTimeRemaining = 0.0f;
}

uint8 FSavedMove_ShooterCharacter::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	/* 
	FLAG_Custom_0		= 0x10, // Unused
	FLAG_Custom_1		= 0x20, // Unused
	FLAG_Custom_2		= 0x40, // Unused
	FLAG_Custom_3		= 0x80, // Unused
	*/
	
	return Result;
}

bool FSavedMove_ShooterCharacter::CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* Character, float MaxDelta) const
{
	const FSavedMove_ShooterCharacter* NewMove = static_cast<const FSavedMove_ShooterCharacter*>(NewMovePtr.Get());

	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(Character);

	// As an optimization, check if the engine can combine saved moves.
	if (bWallrunWantsToUnstick != NewMove->bWallrunWantsToUnstick)
	{
		return false;
	}

	// TIMERS
	// Don't combine on changes to/from zero WantsToUnstickTime.
	if ((WantsToUnstickTimeRemaining == 0.f) != (NewMove->WantsToUnstickTimeRemaining == 0.f))
	{
		return false;
	}

	if ((WallRunTimeRemaining == 0.f) != (NewMove->WallRunTimeRemaining == 0.f))
	{
		return false;
	}

	if ((WallRunCooldownLeftTimeRemaining == 0.f) != (NewMove->WallRunCooldownLeftTimeRemaining == 0.f))
	{
		return false;
	}

	if ((WallRunCooldownRightTimeRemaining == 0.f) != (NewMove->WallRunCooldownRightTimeRemaining == 0.f))
	{
		return false;
	}

	if (WallRunSide != NewMove->WallRunSide) {
		return false;
	}

	if (WallRunState != NewMove->WallRunState) {
		return false;
	}


	if (!WallRunWallNormal.Equals(NewMove->WallRunWallNormal, WallNormalThresholdCombine))
	{
		return false;
	}

	if (ShooterCharacter == nullptr) {
		UE_LOG(LogTemp, Warning, TEXT("FSavedMove_ShooterCharacter::CanCombineWith - Failed to get ShooterCharacter, Saved move combination is disabled."));
		return false;
	}

	UShooterCharacterMovement* MovementComp = Cast<UShooterCharacterMovement>(ShooterCharacter->GetMovementComponent());
	if (MovementComp == nullptr) {
		UE_LOG(LogTemp, Warning, TEXT("FSavedMove_ShooterCharacter::CanCombineWith - Failed to get ShooterCharacterMovement, Saved move combination is disabled."));
		return false;
	}

	if ((CurrentWallRunEndGravity == MovementComp->WallRunGravityEndState) != (NewMove->WallRunCooldownRightTimeRemaining == MovementComp->WallRunGravityEndState))
	{
		return false;
	}
	
	return Super::CanCombineWith(NewMovePtr, Character, MaxDelta);
}

void FSavedMove_ShooterCharacter::CombineWith(const FSavedMove_Character* OldMove, ACharacter* InCharacter, APlayerController* PC, const FVector& OldStartLocation)
{
	const FSavedMove_ShooterCharacter* OldMoveShooter = static_cast<const FSavedMove_ShooterCharacter*>(OldMove);

	UShooterCharacterMovement* charMov = static_cast<UShooterCharacterMovement*>(InCharacter->GetCharacterMovement());
	if (charMov)
	{
		// Roll back timers. SetMoveFor() will copy them to the saved move. We miss a tick, but delta will be greater
		charMov->WantsToUnstickTimeRemaining = OldMoveShooter->WantsToUnstickTimeRemaining;
		charMov->WallRunTimeRemaining = OldMoveShooter->WallRunTimeRemaining;
		charMov->WallRunCooldownLeftTimeRemaining = OldMoveShooter->WallRunCooldownLeftTimeRemaining;
		charMov->WallRunCooldownRightTimeRemaining = OldMoveShooter->WallRunCooldownRightTimeRemaining;


		// Normals are close enough, but we get the average of them anyway
		charMov->WallRunWallNormal = ((WallRunWallNormal + OldMoveShooter->WallRunWallNormal) / 2.0f).GetSafeNormal();
	}

	Super::CombineWith(OldMoveShooter, InCharacter, PC, OldStartLocation);
}

void FSavedMove_ShooterCharacter::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UShooterCharacterMovement* charMov = static_cast<UShooterCharacterMovement*>(Character->GetCharacterMovement());
	if (charMov)
	{
		// Copy values into the saved move

		// Wallrunning
		bWallrunWantsToUnstick = charMov->bWallrunWantsToUnstick;
		WallRunTimeRemaining = charMov->WallRunTimeRemaining;
		WallRunCooldownLeftTimeRemaining = charMov->WallRunCooldownLeftTimeRemaining;
		WallRunCooldownRightTimeRemaining = charMov->WallRunCooldownRightTimeRemaining;
		WantsToUnstickTimeRemaining = charMov->WantsToUnstickTimeRemaining;
		WallRunSide = charMov->WallRunSide;
		WallRunWallNormal = charMov->WallRunWallNormal;
		CurrentWallRunEndGravity = charMov->CurrentWallRunEndGravity;
		WallRunState = charMov->WallRunState;
	}
}

void FSavedMove_ShooterCharacter::PrepMoveFor(class ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UShooterCharacterMovement* charMov = Cast<UShooterCharacterMovement>(Character->GetCharacterMovement());
	if (charMov)
	{
		// Copy values out of the saved move

		// Wallrunning
		charMov->bWallrunWantsToUnstick = bWallrunWantsToUnstick;
		charMov->WantsToUnstickTimeRemaining = WantsToUnstickTimeRemaining;
		charMov->WallRunTimeRemaining = WallRunTimeRemaining;
		charMov->WallRunCooldownLeftTimeRemaining = WallRunCooldownLeftTimeRemaining;
		charMov->WallRunCooldownRightTimeRemaining = WallRunCooldownRightTimeRemaining;
		charMov->WallRunSide = WallRunSide;
		charMov->WallRunWallNormal = WallRunWallNormal;
		charMov->CurrentWallRunEndGravity = CurrentWallRunEndGravity;
		charMov->WallRunState = WallRunState;

	}
}

bool FSavedMove_ShooterCharacter::IsImportantMove(const FSavedMovePtr& LastAckedMove) const
{
	const FSavedMove_ShooterCharacter* NewMove = static_cast<const FSavedMove_ShooterCharacter*>(LastAckedMove.Get());

	if (bWallrunWantsToUnstick != NewMove->bWallrunWantsToUnstick)
	{
		return true;
	}

	return Super::IsImportantMove(LastAckedMove);
}

FNetworkPredictionData_Client_ShooterCharacter::FNetworkPredictionData_Client_ShooterCharacter(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{

}

FSavedMovePtr FNetworkPredictionData_Client_ShooterCharacter::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_ShooterCharacter());
}

bool FShooterCharacterNetworkMoveData::Serialize(
	UCharacterMovementComponent& CharacterMovement, FArchive& Ar,
	UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	bool bSuperSuccess = Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);
	SerializeOptionalValue<bool>(Ar.IsSaving(), Ar, bWantsToUnstick, false);

	return bSuperSuccess && !Ar.IsError();
}

void FShooterCharacterNetworkMoveData::ClientFillNetworkMoveData(
	const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	Super::ClientFillNetworkMoveData(ClientMove, MoveType);
	const FSavedMove_ShooterCharacter& Move = static_cast<const FSavedMove_ShooterCharacter&>(ClientMove);

	bWantsToUnstick = Move.bWallrunWantsToUnstick;
}

FShooterCharacterNetworkMoveDataContainer::FShooterCharacterNetworkMoveDataContainer() : Super()
{
	NewMoveData = &BFDefaultMoveData[0];
	PendingMoveData = &BFDefaultMoveData[1];
	OldMoveData = &BFDefaultMoveData[2];
}
