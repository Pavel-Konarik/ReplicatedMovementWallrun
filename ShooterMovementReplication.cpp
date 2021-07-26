// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterMovementReplication.h"
#include "ShooterCharacterMovement.h"
#include <GameFramework/Character.h>


void FSavedMove_My::Clear()
{
	Super::Clear();

	// Clear all values
	bWallrunWantsToUnstick = 0;
	WantsToUnstickTime = 0.0f;
	WallRunSide = EWallRunSide::Left;
	WallRunWallNormal = FVector::ZeroVector;
	CurrentWallRunEndGravity = 1.0f;
	WallRunState = EWallRunState::End;
}

uint8 FSavedMove_My::GetCompressedFlags() const
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

bool FSavedMove_My::CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* Character, float MaxDelta) const
{
	const FSavedMove_My* NewMove = static_cast<const FSavedMove_My*>(NewMovePtr.Get());

	if (bWallrunWantsToUnstick != NewMove->bWallrunWantsToUnstick)
	{
		return false;
	}

	if (WantsToUnstickTime != NewMove->WantsToUnstickTime)
	{
		return false;
	}

	if (WallRunSide != NewMove->WallRunSide) {
		return false;
	}

	if (WallRunWallNormal != NewMove->WallRunWallNormal) {
		return false;
	}

	if (CurrentWallRunEndGravity != NewMove->CurrentWallRunEndGravity)
	{
		return false;
	}

	if (WallRunState != NewMove->WallRunState) {
		return false;
	}

	return Super::CanCombineWith(NewMovePtr, Character, MaxDelta);
}

void FSavedMove_My::CombineWith(const FSavedMove_Character* NewMove, ACharacter* InCharacter, APlayerController* PC, const FVector& OldStartLocation)
{
	const FSavedMove_My* NewMove2 = static_cast<const FSavedMove_My*>(NewMove);

	bWallrunWantsToUnstick = NewMove2->bWallrunWantsToUnstick;
	WantsToUnstickTime = NewMove2->WantsToUnstickTime;

	Super::CombineWith(NewMove2, InCharacter, PC, OldStartLocation);
}

void FSavedMove_My::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UShooterCharacterMovement* charMov = static_cast<UShooterCharacterMovement*>(Character->GetCharacterMovement());
	if (charMov)
	{
		// Copy values into the saved move

		// Wallrunning
		bWallrunWantsToUnstick = charMov->bWallrunWantsToUnstick;
		WantsToUnstickTime = charMov->WantsToUnstickTime;
		WallRunSide = charMov->WallRunSide;
		WallRunWallNormal = charMov->WallRunWallNormal;
		CurrentWallRunEndGravity = charMov->CurrentWallRunEndGravity;
		WallRunState = charMov->WallRunState;
	}
}

void FSavedMove_My::PrepMoveFor(class ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UShooterCharacterMovement* charMov = Cast<UShooterCharacterMovement>(Character->GetCharacterMovement());
	if (charMov)
	{
		// Copy values out of the saved move

		// Wallrunning
		charMov->bWallrunWantsToUnstick = bWallrunWantsToUnstick;
		charMov->WantsToUnstickTime = WantsToUnstickTime;
		charMov->WallRunSide = WallRunSide;
		charMov->WallRunWallNormal = WallRunWallNormal;
		charMov->CurrentWallRunEndGravity = CurrentWallRunEndGravity;
		charMov->WallRunState = WallRunState;
	}
}

bool FSavedMove_My::IsImportantMove(const FSavedMovePtr& LastAckedMove) const
{
	const FSavedMove_My* NewMove = static_cast<const FSavedMove_My*>(LastAckedMove.Get());

	if (bWallrunWantsToUnstick != NewMove->bWallrunWantsToUnstick)
	{
		return true;
	}

	return Super::IsImportantMove(LastAckedMove);
}

FNetworkPredictionData_Client_My::FNetworkPredictionData_Client_My(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{

}

FSavedMovePtr FNetworkPredictionData_Client_My::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_My());
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
	const FSavedMove_My& Move = static_cast<const FSavedMove_My&>(ClientMove);

	bWantsToUnstick = Move.bWallrunWantsToUnstick;
}

FBFCharacterNetworkMoveDataContainer::FBFCharacterNetworkMoveDataContainer() : Super()
{
	NewMoveData = &BFDefaultMoveData[0];
	PendingMoveData = &BFDefaultMoveData[1];
	OldMoveData = &BFDefaultMoveData[2];
}
