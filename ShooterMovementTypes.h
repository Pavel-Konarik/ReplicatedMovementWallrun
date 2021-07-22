// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShooterMovementTypes.generated.h"


UENUM(BlueprintType)
enum class EWallRunSide : uint8
{
	Left 	UMETA(DisplayName = "Left", ToolTip = "Running along the left side of a wall"),
	Right	UMETA(DisplayName = "Right", ToolTip = "Running along the right side of a wall"),
};


/**
 * WallRun is divided into 3 states.
 *  - Start - Character is still ascending rapidly (going upwards on the wall)
 *  - Mid - Character has low gravity and duration timer has started
 *  - End - Duration timer finished, gravity is increased to force character to fall
 */
UENUM(BlueprintType)
enum class EWallRunState : uint8
{
	Start 		UMETA(DisplayName = "Left", ToolTip = "Before reaching apex"),
	Mid			UMETA(DisplayName = "Right", ToolTip = "After apex, going forward without gravity"),
	End			UMETA(DisplayName = "End", ToolTip = "Increasing gravity to force fall"),
};

USTRUCT(BlueprintType)
struct FWallRunJumpSettings
{
	GENERATED_BODY()

		// Jump Forward velocity
		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float ForwardVelocity;

	// Jump Z Velocity
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float UpVelocity;

	FWallRunJumpSettings()
	{
		ForwardVelocity = 600.0f;
		UpVelocity = 900.0f;
	}

	FWallRunJumpSettings(float InForwardVelocity, float InUpVelocity)
	{
		ForwardVelocity = InForwardVelocity;
		UpVelocity = InUpVelocity;
	}
};

/** Custom movement modes for Characters. */
UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_WallRunning   UMETA(DisplayName = "WallRunning"),
	CMOVE_MAX			UMETA(Hidden),
};