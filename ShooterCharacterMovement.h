// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * Movement component meant for use with Pawns.
 */

#pragma once
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterMovementReplication.h"
#include "ShooterMovementTypes.h"
#include "ShooterCharacterMovement.generated.h"


/**
 * Modified functions in character:
 *		AWallrunCharacter::MoveForward - 
 *		AWallrunCharacter::MoveRight
 *		AWallrunCharacter::CanJumpInternal_Implementation
 *	
 */

UCLASS()
class UShooterCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UShooterCharacterMovement(const FObjectInitializer& ObjectInitializer);

	friend class FSavedMove_ShooterCharacter;

	FShooterCharacterNetworkMoveDataContainer NetworkMoveDataContainer;


public:
	// The ground speed when running
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "My Character Movement|Grounded")
	float RunSpeed = 900.0f;
	// The ground speed when sprinting
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "My Character Movement|Grounded")
	float SprintSpeed = 800.0f;
	// The acceleration when running
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "My Character Movement|Grounded")
	float RunAcceleration = 2048.0f;
	// The acceleration when sprinting
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "My Character Movement|Grounded")
	float SprintAcceleration = 2048.0f;

#pragma region WallRun Defaults
public:
	// The player's velocity while wall running
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running")
	float WallRunSpeed = 1200.0f;

	// TODO: DESC
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running")
	float WallRunAcceleration = 2048.0f;

	/**  Deceleration when Wall Running and not applying acceleration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running")
	float BrakingDecelerationWallRunning = 400.0f;

	// The player's velocity while wall running (applied per side)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running")
	float WallRunCooldown = 0.5f;

	/**
	 *  For how long does an "unstick" button need to be pressed for (in seconds) before the character unsticks from wall
	 * Unstick button is button representing the opposite direction of WallRun normal (when wallrunning right wall, it would be A (WASD), if wallrunning left wall, it would be D)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running")
	float UnstickFromWallTimeThreshold = 0.15f;
	

	/** If WallRun is finite, gravity increases greatly after WallRunDuration period */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Duration")
	bool bIsWallRunInfinite = false;

	/** If WallRun is finite, gravity increases greatly after WallRunDuration period */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Duration", meta = (EditCondition = "!bIsWallRunInfinite"))
	float WallRunDuration = 1.7f;



	/** Jump setting swhen player jumps same way (narrow angle) to wallrun direction */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Jump")
	FWallRunJumpSettings WallRunForwardJump;

	/** Jump settings when player jumps at 90 degree angle to wall direction (away from the wall) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Jump")
	FWallRunJumpSettings WallRunSideJump;

	// Minimum Angle to launch character off the wall (When looking at the wall, we still launch character n degrees off the wall)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Jump")
	float MinimumWallrunJumpAngle = 15.0f;

	// Maximum Angle to launch character off the wall
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Jump")
	float MaximumWallrunJumpAngle = 90.0f;



	// Length of the trace to detect wall
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Wall Detection")
	float CameraTiltWallDistance = 200.0f;
	
	/**  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Camera Tilt")
	bool bShouldTiltCamera = true;

	/**  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Camera Tilt", meta = (EditCondition = bShouldTiltCamera))
	float CameraMaxTilt = 10.0f;

	/** How quickly (in seconds to tilt from no tilt to full tilt) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Camera Tilt", meta = (EditCondition = bShouldTiltCamera))
	float CameraTiltSpeed = 0.2f;
	

	// Length of the trace to detect wall
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Wall Detection")
	float WallDetectDistance = 100.0f;

	/** Number of rays to cast per side when detecting a nearby wall */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Wall Detection")
	int32 NumberOfRaysPerSide = 7;

	/** We trace on 2 levels (top and bottom). This value determines the offset from middle of the character. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Wall Detection")
	float FirstTraceTopOffset = 50.0f;

	/** We trace on 2 levels (top and bottom). This value determines the offset from middle of the character for lower level (fallback) trace. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Wall Detection")
	float FallbackTraceTopOffset = -200.0f;



	/** Gravity scale before apex is reached (when character is sliding up) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Gravity")
	float WallRunGravityScaleUp = 0.75f;

	/** Gravity scale after apex is reached  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Gravity")
	float WallRunGravityMidState = 0.11f;

	/** Gravity scale after apex is reached  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Gravity")
	float WallRunGravityEndState = 0.7f;

	/** How quickly (in seconds) is WallRunGravityEndState applied after transition to end state. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Gravity")
	float WallRunGravityEndStateApplySpeed = 0.5f;

	/** The maximum required Velocity.Z needed to transition from State::Start to State::Mid */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Gravity")
	float WallRunMidZVelocityThreshold = 130.0f;

	

	/** Should increase gravity if character is going slowly? */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Gravity")
	bool bScaleWallRunGravityWithSpeed = true;

	/** The fraction of WallRunSpeed to start increasing gravity. When character moves slow, increase the gravity */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Gravity", meta = (EditCondition = bScaleWallRunGravityWithSpeed))
	float ScaleWallRunGravityStart = 0.6f;

	/** Gravity when Scaling with speed is enabled and character is moving slowly (0 velocity) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running|Gravity", meta = (EditCondition = bScaleWallRunGravityWithSpeed))
	float WallRunGravityScaleSlow = 1.0f;



	/* Z Velocity to be set when character starts new wallrun */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running")
	float WallRunStartZVelocity = 150.0f;

	// Amount of constant force/velocity to apply to stick to the wall
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running")
	float WallRunPushVelocity = 1600.0f;

	/* The minimum angle (from player forward vector) to accept as valid wallrun start 
	This prevents wallrunning when we back to the wall. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running")
	float WallRunStartMaxAngle = 120.0f;
	
	/** Should wallrun start be prevented if character is moving backwards */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running")
	bool bPreventWallRunIfMovingBackwards = true;

	
	/** Push when player requests unsticking from wall */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Wall Running")
	float WallRunUnstickVelocity = 300.0f;

#pragma endregion

#pragma region Sprinting Functions
public:

#pragma endregion


#pragma region Networking

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;

#pragma endregion


#pragma region Wall Running 
	//////////////////////////////////////////////////////////////////////////
	// Camera tilt

	/** Is character close enough to wall to trigger camera tilt */
	bool bIsCloseToWallToTiltCamera = false;

	/** Direction of current camera tilt */
	EWallRunSide CameraTiltSide = EWallRunSide::Left;

	/** Camera tilt alpha/multiplier ranging from -1.0 (full left tilt) to 1.0 (full right tilt) */
	float CurrentCameraTiltAlpha = 0.0f;
	
	/** Returns roll angle by which should the camera be titled by (from -X [left] to +X [right]) */
	float GetCurrentCameraTilt();


	//////////////////////////////////////////////////////////////////////////
	// Cooldown

	void StartWallRunCooldown(EWallRunSide Side);
	bool IsWallRunOnCooldown(EWallRunSide Side);

	float WallRunCooldownLeftTimeRemaining = 0.0f;
	float WallRunCooldownRightTimeRemaining = 0.0f;

	//////////////////////////////////////////////////////////////////////////
	// Unstick from wall

	// [client + server] Replicated via saved moved
	uint8 bWallrunWantsToUnstick : 1;
	
	// Current time of how long WantsToUnstick was pressed for
	float WantsToUnstickTimeRemaining = 0.0f;
	
	// Start short timer before calling UnstickFromWall_Internal which performs the unstick
	void UnstickFromWallPressed();
	
	// Clears the timer time and sets wantstounstick to 0
	void ResetUnstickFromWall();
	
	/** Performs the actual "unsticking" from wall */
	UFUNCTION()
	void UnstickFromWall_Internal();
	

	//////////////////////////////////////////////////////////////////////////
	// Generic Wallrun

	// Checks if wall is close enough and cooldown is down
	bool CanStartWallRunSide(EWallRunSide Side, FVector& OutWallNormal, FVector& OutImpactPoint);

	/** Starts wall running immediately for given side. WallRunNormal and Trace point are stored for physics calculations and debugging respectively */
	void StartWallRunning(EWallRunSide Side, FVector InWallNormal, FVector InWallRunTraceImpactPoint);

	/** Ends wallrun and transitions to fall state */
	void StopWallRunning();

	/** Important function - Traces from character body outwards in specific angles and takes 
	* first impact point (the smallest angle from actor forward vector) and outputs if we hit any wall at all and 
	* if so, a normal of that hit wall surface (only XY axis)
	*/
	bool TraceNearbyForWalls(EWallRunSide Side, bool bFallbackToFeetLevel, FVector& OutNormal, FVector& OutImpactPoint);

	/** Returns the current gravity scale. This changes based on state, time etc. */
	float GetWallRunGravityScale();

	/** Is character currently performing a WallRun? */
	bool IsWallRunning();

	/** Is character currently performing a WallRun for a given side? */
	bool IsWallRunningSide(EWallRunSide Side);

	// The side of the wall the player is running on.
	EWallRunSide WallRunSide;

	// How long till the wallrun transitions to END state. (Timer)
	float WallRunTimeRemaining = 0.0f;

	/** Timer start only when transiting from START -> MID state where the "proper" wallrun begins */
	bool bIsWallRunDurationTimerStarted = false;

	// Important - Is used for calculations such as way of wallrunning (forward vector)
	FVector WallRunWallNormal;
	
	// Used for debugging
	FVector WallRunTraceImpactPoint;

	// When ending wallrun (last stage), gravity in gradually increasing. This is the currant gravity value
	float CurrentWallRunEndGravity = 0.0f;

	// Current state of wallrun
	EWallRunState WallRunState = EWallRunState::Start;

	/** Returns a direction indicating the forward direction of WallRun (expected to be parallel with the wall) */
	FVector GetWallRunForwardDirection();

	/** Returns a direction indicating the forward direction of WallRun with explicitely given wall normal and wallrun side. */
	FVector GetWallRunForwardDirection(EWallRunSide Side, FVector WallNormal);

	/** Returns the acceleration when wallrunning */
	FVector GetWallRunLateralAcceleration(float deltaTime);

	/** Timer has expired, move to end state of wallrun */
	void TransitionWallRunToEndState();

	/** IMPORTANT: This is where the heavy lifting is done. This function is heavely inspired by PhysFalling but modified to work for wallrunning. It is responsible for moving character along the wall */
	void PhysWallRunning(float deltaTime, int32 Iterations);

	/** Perform a jump from wall */
	void DoWallRunJump(bool bReplayingMoves);


#pragma endregion

#pragma region Overrides
protected:
	virtual void BeginPlay() override;
	/** Update the character state in PerformMovement right before doing the actual position change */
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds);

	/** Update the character state in PerformMovement after the position change. Some rotation updates happen after this. */
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds);

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxBrakingDeceleration() const override;
	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;
#pragma endregion



#pragma region Private Variables



#pragma endregion

	bool DoJump(bool bReplayingMoves) override;

protected:
	void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	
	
};
