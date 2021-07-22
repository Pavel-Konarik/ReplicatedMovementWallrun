// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterCharacterMovement.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "Kismet/KismetMathLibrary.h"
#include <DrawDebugHelpers.h>
#include "Camera/CameraComponent.h"
#include <Components/CapsuleComponent.h>
#include "ShooterMovementReplication.h"


int32 CVar_WallRun_ShowAll = 0;
static FAutoConsoleVariableRef CVarWallRunShowForces(TEXT("WallRun.ShowAll"), CVar_WallRun_ShowAll,
	TEXT("Show all forces and events during WallRun movement"), ECVF_Default);

int32 CVar_WallRun_ShowCharacterCapsule = 0;
static FAutoConsoleVariableRef CVarWallRunShowCharacterCapsule(TEXT("WallRun.ShowCharacterCapsule"), CVar_WallRun_ShowCharacterCapsule,
	TEXT("Show ghost of character capsule during WallRunning"), ECVF_Default);

int32 CVar_WallRun_ShowJumps = 0;
static FAutoConsoleVariableRef CVarWallRunShowJumps(TEXT("WallRun.ShowJumps"), CVar_WallRun_ShowJumps,
	TEXT("Show [red] direction of WallRun jumps, [green] last touched point, [blue] character capsule"), ECVF_Default);

int32 CVar_WallRun_ShowRunForwardVector = 0;
static FAutoConsoleVariableRef CVarWallRunShowForwardVector(TEXT("WallRun.ShowRunForwardVector"), CVar_WallRun_ShowRunForwardVector,
	TEXT("Show the forward direction of Wall Run (expected to be parallel with the wall)"), ECVF_Default);

int32 CVar_WallRun_ShowWallNormal = 0;
static FAutoConsoleVariableRef CVarWallRunWallNormal(TEXT("WallRun.ShowWallNormal"), CVar_WallRun_ShowWallNormal,
	TEXT("Show the normal vector of wall face character is currently running on"), ECVF_Default);

int32 CVar_WallRun_ShowGravity = 0;
static FAutoConsoleVariableRef CVarWallRunShowGravity(TEXT("WallRun.ShowGravity"), CVar_WallRun_ShowGravity,
	TEXT("Show the amount of gravity applied during Wall Run"), ECVF_Default);

int32 CVar_WallRun_ShowWallPush = 0;
static FAutoConsoleVariableRef CVarWallRunWallPushVelocity(TEXT("WallRun.ShowWallPush"), CVar_WallRun_ShowWallPush,
	TEXT("Show the velocity and direction applied for character to stick to a wall"), ECVF_Default);

int32 CVar_WallRun_ShowAcceleration = 0;
static FAutoConsoleVariableRef CVarWallRunWallAcceleration(TEXT("WallRun.ShowAcceleration"), CVar_WallRun_ShowAcceleration,
	TEXT("Show direction of acceleration applied to this character. It is expected to be parallel with the wall if character is moving forward"), ECVF_Default);

int32 CVar_WallRun_ShowState = 0;
static FAutoConsoleVariableRef CVarWallRunWallState(TEXT("WallRun.ShowState"), CVar_WallRun_ShowState,
	TEXT("Shows character capsule coloured differently for each state. Start [green], Mid [yellow], End [red]"), ECVF_Default);

//----------------------------------------------------------------------//
// UPawnMovementComponent
//----------------------------------------------------------------------//
UShooterCharacterMovement::UShooterCharacterMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WallRunForwardJump = FWallRunJumpSettings(1600.0f, 600.0f);
	WallRunSideJump = FWallRunJumpSettings(700.0f, 900.0f);

	SetNetworkMoveDataContainer(NetworkMoveDataContainer);
}
FNetworkPredictionData_Client* UShooterCharacterMovement::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		// Return our custom client prediction class instead
		UShooterCharacterMovement* MutableThis = const_cast<UShooterCharacterMovement*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_ShooterCharacter(*this);
	}

	return ClientPredictionData;
}

void UShooterCharacterMovement::BeginPlay()
{
	Super::BeginPlay();
}

void UShooterCharacterMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Handle camera tilting
	if (bShouldTiltCamera)
	{
		if ((bIsCloseToWallToTiltCamera || IsWallRunning()) && (IsFalling() || IsWallRunning())) {
			if (IsWallRunning()) {
				CameraTiltSide = WallRunSide;
			}

			if (CameraTiltSide == EWallRunSide::Left) {
				CurrentCameraTiltAlpha = FMath::Max(CurrentCameraTiltAlpha - ((1.0f / CameraTiltSpeed) * DeltaTime), -1.0f);
			}
			else {
				CurrentCameraTiltAlpha = FMath::Min(CurrentCameraTiltAlpha + ((1.0f / CameraTiltSpeed) * DeltaTime), 1.0f);
			}
		}
		else {
			// No longer wallrunning, tilt back to no-tilt
			if (CurrentCameraTiltAlpha > 0) {
				CurrentCameraTiltAlpha = FMath::Max(CurrentCameraTiltAlpha - ((1.0f / CameraTiltSpeed) * DeltaTime), 0.0f);
			}
			else if (CurrentCameraTiltAlpha < 0) {
				CurrentCameraTiltAlpha = FMath::Min(CurrentCameraTiltAlpha + ((1.0f / CameraTiltSpeed) * DeltaTime), 0.0f);
			}
		}
	}
}

void UShooterCharacterMovement::MoveAutonomous(float ClientTimeStamp, float DeltaTime,
	uint8 CompressedFlags, const FVector& NewAccel)
{
	FShooterCharacterNetworkMoveData* Move = static_cast<FShooterCharacterNetworkMoveData*>(GetCurrentNetworkMoveData());
	if (Move != nullptr)
	{
		bWallrunWantsToUnstick = Move->bWantsToUnstick;
	}

	Super::MoveAutonomous(
		ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}



void UShooterCharacterMovement::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	// TODO: Move this logic to pre-tick or something, since this is effecting the next frame
	APawn* Pawn = GetPawnOwner();
	if (Pawn == nullptr) {
		return;
	}


	// Update wallrunning state
	if (IsFalling() || IsWallRunning())
	{
		if (!IsWallRunning()) {
			// Not wallrunning and falling only
			// Trace line for nearby walls
			FVector WallNormal;
			FVector ImpactPoint;
			if (CanStartWallRunSide(EWallRunSide::Left, WallNormal, ImpactPoint)) {
				StartWallRunning(EWallRunSide::Left, WallNormal, ImpactPoint);
			}
			else if (CanStartWallRunSide(EWallRunSide::Right, WallNormal, ImpactPoint)) {
				StartWallRunning(EWallRunSide::Right, WallNormal, ImpactPoint);
			}

		}
		else if (IsWallRunningSide(EWallRunSide::Left)) {
			// Already wallrunning Left
			// If we are no longer running at wall
			// If we are still running, this will update the wall normal
			if (TraceNearbyForWalls(EWallRunSide::Left, true, WallRunWallNormal, WallRunTraceImpactPoint) == false) {
				StopWallRunning();
			}
		}
		else {
			// Already wallrunning right
			// If we are no longer running at wall
			// If we are still running, this will update the wall normal
			if (TraceNearbyForWalls(EWallRunSide::Right, true, WallRunWallNormal, WallRunTraceImpactPoint) == false) {
				StopWallRunning();
			}
		}
	}
	

	// Wallrunnign Update state
	if (IsWallRunning() && WallRunState == EWallRunState::Start && Velocity.Z <= WallRunMidZVelocityThreshold)
	{
		WallRunState = EWallRunState::Mid;
		// Set timer for duration of "Mid" section of WallRun
		if (!bIsWallRunInfinite) {
			bIsWallRunDurationTimerStarted = true;
		}
	}

	
	// Update WallRun Timers
	
	// If in end state, gradually increase gravity until desired gravity is reached
	if (IsWallRunning() && WallRunState == EWallRunState::End)
	{
		if (CurrentWallRunEndGravity < WallRunGravityEndState)
		{
			CurrentWallRunEndGravity = FMath::Min(WallRunGravityEndState, CurrentWallRunEndGravity + ((1.0f / WallRunGravityEndStateApplySpeed) * DeltaSeconds));
		}
	}

	// Unstick Timer
	if (bWallrunWantsToUnstick)
	{
		WantsToUnstickTimeRemaining = FMath::Max(0.0f, WantsToUnstickTimeRemaining - DeltaSeconds);
		if (WantsToUnstickTimeRemaining <= 0.0f)
		{
			UnstickFromWall_Internal();
		}
	}
	else {
		WantsToUnstickTimeRemaining = UnstickFromWallTimeThreshold;
	}

	// WallRun Cooldowns
	if (WallRunCooldownLeftTimeRemaining > 0.0f)
	{
		WallRunCooldownLeftTimeRemaining = FMath::Max(0.0f, WallRunCooldownLeftTimeRemaining - DeltaSeconds);
	}

	if (WallRunCooldownRightTimeRemaining > 0.0f)
	{
		WallRunCooldownRightTimeRemaining = FMath::Max(0.0f, WallRunCooldownRightTimeRemaining - DeltaSeconds);
	}
	// Wallrun duration timer
	if (bIsWallRunDurationTimerStarted && IsWallRunning() && WallRunTimeRemaining > 0.0f)
	{
		WallRunTimeRemaining = FMath::Max(0.0f, WallRunTimeRemaining - DeltaSeconds);
		if (WallRunTimeRemaining <= 0.0f && WallRunState != EWallRunState::End)
		{
			TransitionWallRunToEndState();
		}
	}
}

void UShooterCharacterMovement::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);
}

bool UShooterCharacterMovement::DoJump(bool bReplayingMoves)
{
	// Implementation taken from CharacterMovementComponent::DoJump but added chase for wallrunning
	if(CharacterOwner){
		if (IsWallRunning()) 
		{
			DoWallRunJump(bReplayingMoves);
		}
		else if(CharacterOwner->CanJump()) // Default jump
		{
			// Don't jump if we can't move up/down.
			if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
			{
				Velocity.Z = FMath::Max(Velocity.Z, JumpZVelocity);
				SetMovementMode(MOVE_Falling);
				return true;
			}
		}
	}

	return false;
}

void UShooterCharacterMovement::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);


}

void UShooterCharacterMovement::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}


void UShooterCharacterMovement::PhysCustom(float deltaTime, int32 Iterations)
{
	switch (CustomMovementMode)
	{
	case ECustomMovementMode::CMOVE_WallRunning:
	{
		PhysWallRunning(deltaTime, Iterations);
		break;
	}
	}

	// Not sure if this is needed
	Super::PhysCustom(deltaTime, Iterations);
}


float UShooterCharacterMovement::GetMaxSpeed() const
{
	switch (MovementMode)
	{
	case MOVE_Walking:
	case MOVE_NavWalking:
	{
		if (IsCrouching())
		{
			return MaxWalkSpeedCrouched;
		}
		else
		{

			return RunSpeed;
		}
	}
	case MOVE_Falling:
		return MaxWalkSpeed;
	case MOVE_Swimming:
		return MaxSwimSpeed;
	case MOVE_Flying:
		return MaxFlySpeed;
	case MOVE_Custom:
		switch (CustomMovementMode)
		{
		case CMOVE_WallRunning:
		{
			return WallRunSpeed;
		}
		default:
			return MaxCustomMovementSpeed;
		}

	case MOVE_None:
	default:
		return 0.f;
	}
}

float UShooterCharacterMovement::GetMaxAcceleration() const
{
	if (MovementMode == MOVE_Custom)
	{
		switch (CustomMovementMode)
		{
		case CMOVE_WallRunning:
		{
			return WallRunAcceleration;
		}
		default:
			return Super::GetMaxAcceleration();
		}
	}

	return Super::GetMaxAcceleration();
}

float UShooterCharacterMovement::GetMaxBrakingDeceleration() const
{
	if (MovementMode == MOVE_Custom)
	{
		switch (CustomMovementMode)
		{
		case CMOVE_WallRunning:
		{
			return BrakingDecelerationWallRunning;
		}
		default:
			return Super::GetMaxBrakingDeceleration();
		}
	}

	return Super::GetMaxBrakingDeceleration();
}

void UShooterCharacterMovement::ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations)
{

	// If we landed while wall running, make sure we stop wall running
	if (IsWallRunning())
	{
		StopWallRunning();
	}


	Super::ProcessLanded(Hit, remainingTime, Iterations);
}

void UShooterCharacterMovement::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	// Read the values from the compressed flags
	// bWallrunWantsToUnstick = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}



#pragma region WallRun
void UShooterCharacterMovement::DoWallRunJump(bool bReplayingMoves)
{
	// We calculate a deviation angle from the wallrun direction
			// And base the jump direction, velocity (horizontal and vertical) based on that angle
			// More acute angle results in faster jump (but not as much Z) and vise-versa
	FVector PawnForwardVector = GetPawnOwner()->GetActorForwardVector();

	// This is -180 to 180 angle compared to wallrun direction
	float AimAngle = FMath::UnwindDegrees(UKismetMathLibrary::DegAtan2(PawnForwardVector.X, PawnForwardVector.Y) - UKismetMathLibrary::DegAtan2(Velocity.X, Velocity.Y));

	bool bIsFacingIntoWall = false;
	if ((WallRunSide == EWallRunSide::Left && AimAngle < 0.0f) ||
		(WallRunSide == EWallRunSide::Right && AimAngle > 0.0f))
	{
		bIsFacingIntoWall = true;
	}

	AimAngle = FMath::Abs(AimAngle);

	// From 15 (min angle) to 90 (max angle) 
	AimAngle = FMath::Clamp(AimAngle, MinimumWallrunJumpAngle, MaximumWallrunJumpAngle);
	// 0 to 1 from Min Angle to Max Angle
	float AimAngleLerpAlpha = (AimAngle - MinimumWallrunJumpAngle) / MaximumWallrunJumpAngle;

	// Calculate Jump Velocity based on angle
	float NewZVelocity = FMath::Lerp(WallRunForwardJump.UpVelocity, WallRunSideJump.UpVelocity, AimAngleLerpAlpha);
	float NewXYVelocity = FMath::Lerp(WallRunForwardJump.ForwardVelocity, WallRunSideJump.ForwardVelocity, AimAngleLerpAlpha);

	// Set Z velocity
	Velocity.Z = FMath::Max(Velocity.Z, NewZVelocity);

	// Current velocity (the wallrun direction) * New velocity 
	// This results in correct magnitude but it is in direction of wall run, we rotate it later
	FVector2D HorizontalVelocity = FVector2D(Velocity.X, Velocity.Y).GetSafeNormal() * NewXYVelocity;
	Velocity.X = HorizontalVelocity.X;
	Velocity.Y = HorizontalVelocity.Y;

	// Rotate jump direction
	if (WallRunSide == EWallRunSide::Left) {
		Velocity = UKismetMathLibrary::RotateAngleAxis(Velocity, AimAngle, FVector(0.0f, 0.0f, 1.0f));
	}
	else {
		Velocity = UKismetMathLibrary::RotateAngleAxis(Velocity, -AimAngle, FVector(0.0f, 0.0f, 1.0f));
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// Debug Jump Arrow
	if (CVar_WallRun_ShowAll || CVar_WallRun_ShowJumps)
	{
		FVector PawnLoc = GetPawnOwner()->GetActorLocation();

		// Draw where was the last "contact" point
		DrawDebugPoint(GetWorld(), WallRunTraceImpactPoint, 10.0f, FColor::Green, false, 20.0f);
		// Draw the direction and velocity of jump
		DrawDebugDirectionalArrow(GetWorld(), PawnLoc, PawnLoc + Velocity.GetSafeNormal() * 200.0f, 60.f, FColor::Red, false, 20.0f, 0, 2.f);

		// Draw character capsule (If owned by character which as capsule)
		if (ACharacter* Character = Cast<ACharacter>(GetPawnOwner())) {
			UCapsuleComponent* CharacterCapsule = Character->GetCapsuleComponent();
			if (CharacterCapsule) {
				FVector CapsuleLoc = CharacterCapsule->GetComponentLocation();
				float HalfHeight = CharacterCapsule->GetScaledCapsuleHalfHeight();
				float Radius = CharacterCapsule->GetScaledCapsuleRadius();
				FRotator Rotation = CharacterCapsule->GetComponentRotation();

				DrawDebugCapsule(GetWorld(), CapsuleLoc, HalfHeight, Radius, Rotation.Quaternion(), FColor::Blue, false, 20.0f, 0, 0.5f);
			}
		}
	}
#endif
	//UE_LOG(LogTemp, Log, TEXT("WallRun Jump - Player Look Angle: %f, Velocity Lerp: %f, New XY Magniture: %f"), AimAngle, AimAngleLerpAlpha, NewXYVelocity);
	StopWallRunning();
}


void UShooterCharacterMovement::StartWallRunning(EWallRunSide Side, FVector InWallNormal, FVector InWallRunTraceImpactPoint)
{
	// Clean up
	WallRunState = EWallRunState::Start;
	bWallrunWantsToUnstick = false;
	WantsToUnstickTimeRemaining = 0.0f;
	bIsWallRunDurationTimerStarted = false;

	WallRunTimeRemaining = WallRunDuration;
	WallRunSide = Side;
	WallRunWallNormal = InWallNormal;
	WallRunTraceImpactPoint = InWallRunTraceImpactPoint;
	
	Velocity.Z = FMath::Max(Velocity.Z, WallRunStartZVelocity);

	SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_WallRunning);
}

void UShooterCharacterMovement::StopWallRunning()
{
	StartWallRunCooldown(WallRunSide);
	SetMovementMode(EMovementMode::MOVE_Falling);
}

bool UShooterCharacterMovement::CanStartWallRunSide(EWallRunSide Side, FVector& OutWallNormal, FVector& OutImpactPoint)
{
	FVector TestedWallNormal;
	bool bSuccess = IsWallRunOnCooldown(Side) == false && TraceNearbyForWalls(Side, false, TestedWallNormal, OutImpactPoint);
	if (bSuccess) 
	{
		// Check if we are rotated in acceptable angle (Prevents starting wallrun when player back faces wall)
		FVector PawnForwardVector = GetPawnOwner()->GetActorForwardVector();
		FVector RunDirection = GetWallRunForwardDirection(Side, TestedWallNormal);
		// This is -180 to 180 angle compared to wallrun direction
		float AimAngle = FMath::UnwindDegrees(UKismetMathLibrary::DegAtan2(PawnForwardVector.X, PawnForwardVector.Y) - UKismetMathLibrary::DegAtan2(RunDirection.X, RunDirection.Y));
		if (FMath::Abs(AimAngle) > WallRunStartMaxAngle)
		{
			return false;
		}


		// Check if player is not moving backwards
		if(bPreventWallRunIfMovingBackwards) {
			float FacingForwardDot = FVector::DotProduct(Velocity.GetSafeNormal(), PawnForwardVector.GetSafeNormal());
			bool bIsFacingForward = FacingForwardDot > 0.0f;
			if (bIsFacingForward == false) {
				return false;
			}
		}

		// Add additional wallsliding checks here

		// All tests passed
		OutWallNormal = TestedWallNormal;
		return true;
	}


	return false;
}


bool UShooterCharacterMovement::TraceNearbyForWalls(EWallRunSide Side, bool bFallbackToFeetLevel, FVector& OutNormal, FVector& OutImpactPoint)
{
	// Create a TArray of target angles to raycast (Without TArray would be cleaner, but this is ready if we need to manually add some odd angles)
	float TargetAngle = 180.0f / NumberOfRaysPerSide + 1;
	
	TArray<float> Angles;
	for (int32 i = 1; i < NumberOfRaysPerSide; i++)
	{
		Angles.Add(TargetAngle * i);
	}


	AShooterCharacter* Pawn = Cast<AShooterCharacter>(GetPawnOwner());
	if (Pawn == nullptr) {
		return false;
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Pawn);

	float Direction = Side == EWallRunSide::Left ? -1.0f : 1.0f;
	float Distance = FMath::Max(WallDetectDistance, CameraTiltWallDistance);

	FVector PawnLoc = Pawn->GetActorLocation();
	FVector EndLocPreRotate = Pawn->GetActorForwardVector() * Distance;

	bool bFoundCameraTilt = false;

	for (int32 i = 0; i < Angles.Num(); i++)
	{	
		float Angle = Angles[i];
		FHitResult HitResult;
		FVector EndLoc = UKismetMathLibrary::RotateAngleAxis(EndLocPreRotate, Angle * Direction, FVector(0.0f, 0.0f, 1.0f)) + PawnLoc;
		//UKismetSystemLibrary::LineTraceSingle(this, StartLoc, EndLoc, UEngineTypes::ConvertToTraceType(ECC_Visibility), false, TArray<AActor*>(), EDrawDebugTrace::ForDuration, HitResult, true, Side == EWallRunSide::Left ? FLinearColor::Red : FLinearColor::Green);
		UKismetSystemLibrary::LineTraceSingle(this, PawnLoc + FVector(0.0f, 0.0f, FirstTraceTopOffset), EndLoc + FVector(0.0f, 0.0f, FirstTraceTopOffset), UEngineTypes::ConvertToTraceType(ECC_Visibility), false, TArray<AActor*>(), EDrawDebugTrace::None, HitResult, true);


		if (bFallbackToFeetLevel && HitResult.bBlockingHit == false)
		{
			UKismetSystemLibrary::LineTraceSingle(this, PawnLoc + FVector(0.0f, 0.0f, FallbackTraceTopOffset), EndLoc + FVector(0.0f, 0.0f, FallbackTraceTopOffset), UEngineTypes::ConvertToTraceType(ECC_Visibility), false, TArray<AActor*>(), EDrawDebugTrace::None, HitResult, true);
		}
		
		// Handle Camera Tilt
		// Do we care about camera tilt and is player velocity heading to that hit/wall?
		if (!bFoundCameraTilt && bShouldTiltCamera && HitResult.bBlockingHit && HitResult.Distance <= CameraTiltWallDistance && Pawn->IsFirstPerson())
		{
			float t = 0.0f;
			FVector IntersectionPoint;
			bool bIntersepts = UKismetMathLibrary::LinePlaneIntersection_OriginNormal(PawnLoc, PawnLoc + (Velocity * FVector(1.0f, 1.0f, 0.0f)).GetSafeNormal() * 700.0f, HitResult.Location, HitResult.Normal, t, IntersectionPoint);
			if (bIntersepts)
			{
				bIsCloseToWallToTiltCamera = true;
				CameraTiltSide = Side;
				bFoundCameraTilt = true;
			}
		}

		// If we pre-tilting camera is no longer required
		if (bShouldTiltCamera && !bFoundCameraTilt && bIsCloseToWallToTiltCamera && CameraTiltSide == Side)
		{
			bIsCloseToWallToTiltCamera = false;
		}
		
		// Handle Wallrunning 
		if (HitResult.bBlockingHit && HitResult.Distance <= WallDetectDistance)
		{
			// Priorities forward walls
			OutImpactPoint = HitResult.Location;
			OutNormal = (HitResult.Normal * FVector(1.0f, 1.0f, 0.0f)).GetSafeNormal();
			return true;
		}
		
	}

	return false;
}


void UShooterCharacterMovement::UnstickFromWallPressed()
{
	if (IsWallRunning() && !bWallrunWantsToUnstick)
	{
		WantsToUnstickTimeRemaining = UnstickFromWallTimeThreshold;
		bWallrunWantsToUnstick = true;
	}
}

void UShooterCharacterMovement::UnstickFromWall_Internal()
{
	if (IsWallRunning())
	{
		Velocity += WallRunWallNormal.GetSafeNormal() * WallRunUnstickVelocity;
		StopWallRunning();
	}
}

void UShooterCharacterMovement::ResetUnstickFromWall()
{
	WantsToUnstickTimeRemaining = UnstickFromWallTimeThreshold;
	bWallrunWantsToUnstick = false;
}

float UShooterCharacterMovement::GetWallRunGravityScale()
{
	// Moving up, return specific value
	if (Velocity.Z > WallRunMidZVelocityThreshold) {
		return WallRunGravityScaleUp;
	}

	
	float NewGravityScale = 0.0f;

	// Moving down, start with the default
	if (WallRunState == EWallRunState::Mid)
	{
		NewGravityScale = FMath::Max(NewGravityScale, WallRunGravityMidState);
	}
	else if (WallRunState == EWallRunState::End)
	{
		NewGravityScale = WallRunGravityEndState;
	}

	// Should we increase gravity if moving slowly?
	if (bScaleWallRunGravityWithSpeed) 
	{
		float CurrentSpeed = Velocity.Size2D();
		if (CurrentSpeed < WallRunSpeed * ScaleWallRunGravityStart)
		{
			float Alpha = CurrentSpeed / (WallRunSpeed * ScaleWallRunGravityStart);
			Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
			NewGravityScale = FMath::Max(NewGravityScale, FMath::Lerp(WallRunGravityScaleSlow, WallRunGravityMidState, Alpha));
		}
	}

	return NewGravityScale;
}

bool UShooterCharacterMovement::IsWallRunning()
{
	return MovementMode == MOVE_Custom && CustomMovementMode == ECustomMovementMode::CMOVE_WallRunning && UpdatedComponent;
}

bool UShooterCharacterMovement::IsWallRunningSide(EWallRunSide Side)
{
	return IsWallRunning() && WallRunSide == Side;
}

void UShooterCharacterMovement::StartWallRunCooldown(EWallRunSide Side) 
{
	UWorld* world = GetWorld();
	if (world)
	{
		if (Side == EWallRunSide::Left) {
			WallRunCooldownLeftTimeRemaining = WallRunCooldown;
		}
		else {
			WallRunCooldownRightTimeRemaining = WallRunCooldown;
		}
	}
}

bool UShooterCharacterMovement::IsWallRunOnCooldown(EWallRunSide Side) 
{
	if (Side == EWallRunSide::Left) {
		return WallRunCooldownLeftTimeRemaining > 0.0f;
	}
	return WallRunCooldownRightTimeRemaining > 0.0f;
}

void UShooterCharacterMovement::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

float UShooterCharacterMovement::GetCurrentCameraTilt()
{
	return CurrentCameraTiltAlpha* CameraMaxTilt * -1.0f;
}

FVector UShooterCharacterMovement::GetWallRunForwardDirection()
{
	float WallDirection = WallRunSide == EWallRunSide::Left ? -1.0f : 1.0f;
	return UKismetMathLibrary::RotateAngleAxis(WallRunWallNormal, 90.0f * WallDirection, FVector::UpVector);
}

FVector UShooterCharacterMovement::GetWallRunForwardDirection(EWallRunSide Side, FVector WallNormal)
{
	float WallDirection = Side == EWallRunSide::Left ? -1.0f : 1.0f;
	return UKismetMathLibrary::RotateAngleAxis(WallNormal, 90.0f * WallDirection, FVector::UpVector);
}

FVector UShooterCharacterMovement::GetWallRunLateralAcceleration(float deltaTime)
{
	return FVector(Acceleration.X, Acceleration.Y, 0.f);
}

void UShooterCharacterMovement::TransitionWallRunToEndState()
{
	if (WallRunState != EWallRunState::End) {
		CurrentWallRunEndGravity = WallRunGravityMidState;
		WallRunState = EWallRunState::End;
	}
}

void UShooterCharacterMovement::PhysWallRunning(float deltaTime, int32 Iterations)
{
	// PhysFalling code, but adjusted for wallrunning

	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	
	// Acceleration is replicated

	FVector WallRunAccelerationLocal = GetWallRunLateralAcceleration(deltaTime);

	WallRunAccelerationLocal.Z = 0.f;
	const bool bHasLimitedAirControl = ShouldLimitAirControl(deltaTime, WallRunAccelerationLocal);


	float remainingTime = deltaTime;
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations))
	{
		Iterations++;
		float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
		bJustTeleported = false;

		RestorePreAdditiveRootMotionVelocity();

		const FVector OldVelocity = Velocity;
		// Apply input
		const float MaxDecel = GetMaxBrakingDeceleration();
		
		if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		{
			// Compute Velocity
			{
				// Acceleration = WallRunAcceleration for CalcVelocity(), but we restore it after using it.
				TGuardValue<FVector> RestoreAcceleration(Acceleration, WallRunAccelerationLocal);
				Velocity.Z = 0.f;
				CalcVelocity(timeTick, /*WallRunLateralFriction*/ 0.0f, false, MaxDecel);
				Velocity.Z = OldVelocity.Z;
			}
		}
		
		

		// Apply push to a side where the wall is (stick to the wall)
		FVector PushToStickToWall = WallRunWallNormal * WallRunPushVelocity * timeTick * -1.0f;;
		Velocity += PushToStickToWall;

		// Compute current gravity
		const FVector Gravity(0.f, 0.f, GetGravityZ() * GetWallRunGravityScale());
		float GravityTime = timeTick;

		

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		// Purely debug options in this section
		if (CVar_WallRun_ShowAll || CVar_WallRun_ShowCharacterCapsule || CVar_WallRun_ShowState)
		{
			// Draw character capsule (If owned by character which as capsule)
			if (ACharacter* Character = Cast<ACharacter>(GetPawnOwner())) {
				UCapsuleComponent* CharacterCapsule = Character->GetCapsuleComponent();
				if (CharacterCapsule) {
					FVector CapsuleLoc = CharacterCapsule->GetComponentLocation();
					float HalfHeight = CharacterCapsule->GetScaledCapsuleHalfHeight();
					float Radius = CharacterCapsule->GetScaledCapsuleRadius();
					FRotator Rotation = CharacterCapsule->GetComponentRotation();

					FColor color = FColor::Blue;
					if (CVar_WallRun_ShowState)
					{
						if (WallRunState == EWallRunState::Start)
						{
							color = FColor::Green;
						}
						else if (WallRunState == EWallRunState::Mid)
						{
							color = FColor::Yellow;
						}
						else
						{
							color = FColor::Red;
						}
					}

					DrawDebugCapsule(GetWorld(), CapsuleLoc, HalfHeight, Radius, Rotation.Quaternion(), color, false, 20.0f, 0, 0.5f);
				}
			}
		}
		
		if (CVar_WallRun_ShowAll || CVar_WallRun_ShowRunForwardVector)
		{
			FVector PawnLoc = GetPawnOwner()->GetActorLocation();
			DrawDebugDirectionalArrow(GetWorld(), PawnLoc, PawnLoc + GetWallRunForwardDirection().GetSafeNormal() * 300.0f, 80.f, FColor::Blue, false, 0, 0, 2.f);
		}

		if (CVar_WallRun_ShowAll || CVar_WallRun_ShowGravity)
		{
			FVector PawnLoc = GetPawnOwner()->GetActorLocation();
			DrawDebugDirectionalArrow(GetWorld(), PawnLoc, PawnLoc + (Gravity / 4.0f), 80.f, FColor::Red, false, 20.0f, 0, 1.f);
		}

		if (CVar_WallRun_ShowAll || CVar_WallRun_ShowWallPush)
		{
			FVector PawnLoc = GetPawnOwner()->GetActorLocation();
			DrawDebugDirectionalArrow(GetWorld(), PawnLoc, PawnLoc + PushToStickToWall.GetSafeNormal() * 100.0f, 80.f, FColor::Cyan, false, 20.0f, 0, 1.f);
		}

		if (CVar_WallRun_ShowAll || CVar_WallRun_ShowWallNormal)
		{
			FVector PawnLoc = GetPawnOwner()->GetActorLocation();
			DrawDebugDirectionalArrow(GetWorld(), PawnLoc, PawnLoc + WallRunWallNormal.GetSafeNormal() * 100.0f, 80.f, FColor::Yellow, false, 20.0f, 0, 1.f);
		}

		if (CVar_WallRun_ShowAll || CVar_WallRun_ShowAcceleration)
		{
			FVector PawnLoc = GetPawnOwner()->GetActorLocation();
			DrawDebugDirectionalArrow(GetWorld(), PawnLoc, PawnLoc + Acceleration.GetSafeNormal() * 100.0f, 80.f, FColor::Orange, false, 20.0f, 0, 1.f);
		}
#endif

		// If jump is providing force, gravity may be affected.
		bool bEndingJumpForce = false;
		if (CharacterOwner->JumpForceTimeRemaining > 0.0f)
		{
			// Consume some of the force time. Only the remaining time (if any) is affected by gravity when bApplyGravityWhileJumping=false.
			const float JumpForceTime = FMath::Min(CharacterOwner->JumpForceTimeRemaining, timeTick);
			GravityTime = bApplyGravityWhileJumping ? timeTick : FMath::Max(0.0f, timeTick - JumpForceTime);

			// Update Character state
			CharacterOwner->JumpForceTimeRemaining -= JumpForceTime;
			if (CharacterOwner->JumpForceTimeRemaining <= 0.0f)
			{
				CharacterOwner->ResetJumpState();
				bEndingJumpForce = true;
			}
		}

		// Apply gravity
		Velocity = NewFallVelocity(Velocity, Gravity, GravityTime);

		// See if we need to sub-step to exactly reach the apex. This is important for avoiding "cutting off the top" of the trajectory as framerate varies.
		if (OldVelocity.Z > 0.f && Velocity.Z <= 0.f && NumJumpApexAttempts < MaxJumpApexAttemptsPerSimulation)
		{
			const FVector DerivedAccel = (Velocity - OldVelocity) / timeTick;
			if (!FMath::IsNearlyZero(DerivedAccel.Z))
			{
				const float TimeToApex = -OldVelocity.Z / DerivedAccel.Z;

				// The time-to-apex calculation should be precise, and we want to avoid adding a substep when we are basically already at the apex from the previous iteration's work.
				const float ApexTimeMinimum = 0.0001f;
				if (TimeToApex >= ApexTimeMinimum && TimeToApex < timeTick)
				{
					const FVector ApexVelocity = OldVelocity + DerivedAccel * TimeToApex;
					Velocity = ApexVelocity;
					Velocity.Z = 0.f; // Should be nearly zero anyway, but this makes apex notifications consistent.

					// We only want to move the amount of time it takes to reach the apex, and refund the unused time for next iteration.
					remainingTime += (timeTick - TimeToApex);
					timeTick = TimeToApex;
					Iterations--;
					NumJumpApexAttempts++;
				}
			}
		}

		ApplyRootMotionToVelocity(timeTick);

		// Compute change in position (using midpoint integration method).
		FVector Adjusted = 0.5f * (OldVelocity + Velocity) * timeTick;

		// Special handling if ending the jump force where we didn't apply gravity during the jump.
		if (bEndingJumpForce && !bApplyGravityWhileJumping)
		{
			// We had a portion of the time at constant speed then a portion with acceleration due to gravity.
			// Account for that here with a more correct change in position.
			const float NonGravityTime = FMath::Max(0.f, timeTick - GravityTime);
			Adjusted = (OldVelocity * NonGravityTime) + (0.5f * (OldVelocity + Velocity) * GravityTime);
		}

		// Move
		FHitResult Hit(1.f);
		SafeMoveUpdatedComponent(Adjusted, PawnRotation, true, Hit);

		if (!HasValidData())
		{
			return;
		}

		float LastMoveTimeSlice = timeTick;
		float subTimeTickRemaining = timeTick * (1.f - Hit.Time);

		if (IsSwimming()) //just entered water
		{
			remainingTime += subTimeTickRemaining;
			StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
			return;
		}
		else if (Hit.bBlockingHit)
		{
			if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
			{
				remainingTime += subTimeTickRemaining;
				ProcessLanded(Hit, remainingTime, Iterations);
				return;
			}
			else
			{
				// Compute impact deflection based on final velocity, not integration step.
				// This allows us to compute a new velocity from the deflected vector, and ensures the full gravity effect is included in the slide result.
				Adjusted = Velocity * timeTick;

				// See if we can convert a normally invalid landing spot (based on the hit result) to a usable one.
				if (!Hit.bStartPenetrating && ShouldCheckForValidLandingSpot(timeTick, Adjusted, Hit))
				{
					const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
					FFindFloorResult FloorResult;
					FindFloor(PawnLocation, FloorResult, false);
					if (FloorResult.IsWalkableFloor() && IsValidLandingSpot(PawnLocation, FloorResult.HitResult))
					{
						remainingTime += subTimeTickRemaining;
						ProcessLanded(FloorResult.HitResult, remainingTime, Iterations);
						return;
					}
				}

				HandleImpact(Hit, LastMoveTimeSlice, Adjusted);

				// If we've changed physics mode, abort.
				if (!HasValidData() || !IsWallRunning())
				{
					return;
				}

				// Limit air control based on what we hit.
				// We moved to the impact point using air control, but may want to deflect from there based on a limited air control acceleration.
				FVector VelocityNoAirControl = OldVelocity;
				FVector AirControlAccel = Acceleration;
				if (bHasLimitedAirControl)
				{
					// Compute VelocityNoAirControl
					{
						// Find velocity *without* acceleration.
						TGuardValue<FVector> RestoreAcceleration(Acceleration, FVector::ZeroVector);
						TGuardValue<FVector> RestoreVelocity(Velocity, OldVelocity);
						Velocity.Z = 0.f;
						CalcVelocity(timeTick, 0.0f, false, MaxDecel);
						VelocityNoAirControl = FVector(Velocity.X, Velocity.Y, OldVelocity.Z);
						VelocityNoAirControl = NewFallVelocity(VelocityNoAirControl, Gravity, GravityTime);
					}

					const bool bCheckLandingSpot = false; // we already checked above.
					AirControlAccel = (Velocity - VelocityNoAirControl) / timeTick;
					const FVector AirControlDeltaV = LimitAirControl(LastMoveTimeSlice, AirControlAccel, Hit, bCheckLandingSpot) * LastMoveTimeSlice;
					Adjusted = (VelocityNoAirControl + AirControlDeltaV) * LastMoveTimeSlice;
				}

				const FVector OldHitNormal = Hit.Normal;
				const FVector OldHitImpactNormal = Hit.ImpactNormal;
				FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, OldHitNormal, Hit);

				// Compute velocity after deflection (only gravity component for RootMotion)
				if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
				{
					const FVector NewVelocity = (Delta / subTimeTickRemaining);
					Velocity = HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocityWithIgnoreZAccumulate() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
				}

				if (subTimeTickRemaining > KINDA_SMALL_NUMBER && (Delta | Adjusted) > 0.f)
				{
					// Move in deflected direction.
					SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

					if (Hit.bBlockingHit)
					{
						// hit second wall
						LastMoveTimeSlice = subTimeTickRemaining;
						subTimeTickRemaining = subTimeTickRemaining * (1.f - Hit.Time);

						if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
						{
							remainingTime += subTimeTickRemaining;
							ProcessLanded(Hit, remainingTime, Iterations);
							return;
						}

						HandleImpact(Hit, LastMoveTimeSlice, Delta);

						// If we've changed physics mode, abort.
						if (!HasValidData() || !IsWallRunning())
						{
							return;
						}

						// Act as if there was no air control on the last move when computing new deflection.
						if (bHasLimitedAirControl && Hit.Normal.Z > 0.001f)
						{
							const FVector LastMoveNoAirControl = VelocityNoAirControl * LastMoveTimeSlice;
							Delta = ComputeSlideVector(LastMoveNoAirControl, 1.f, OldHitNormal, Hit);
						}

						FVector PreTwoWallDelta = Delta;
						TwoWallAdjust(Delta, Hit, OldHitNormal);

						// Limit air control, but allow a slide along the second wall.
						if (bHasLimitedAirControl)
						{
							const bool bCheckLandingSpot = false; // we already checked above.
							const FVector AirControlDeltaV = LimitAirControl(subTimeTickRemaining, AirControlAccel, Hit, bCheckLandingSpot) * subTimeTickRemaining;

							// Only allow if not back in to first wall
							if (FVector::DotProduct(AirControlDeltaV, OldHitNormal) > 0.f)
							{
								Delta += (AirControlDeltaV * subTimeTickRemaining);
							}
						}

						// Compute velocity after deflection (only gravity component for RootMotion)
						if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
						{
							const FVector NewVelocity = (Delta / subTimeTickRemaining);
							Velocity = HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocityWithIgnoreZAccumulate() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
						}

						// bDitch=true means that pawn is straddling two slopes, neither of which he can stand on
						bool bDitch = ((OldHitImpactNormal.Z > 0.f) && (Hit.ImpactNormal.Z > 0.f) && (FMath::Abs(Delta.Z) <= KINDA_SMALL_NUMBER) && ((Hit.ImpactNormal | OldHitImpactNormal) < 0.f));
						SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
						if (Hit.Time == 0.f)
						{
							// if we are stuck then try to side step
							FVector SideDelta = (OldHitNormal + Hit.ImpactNormal).GetSafeNormal2D();
							if (SideDelta.IsNearlyZero())
							{
								SideDelta = FVector(OldHitNormal.Y, -OldHitNormal.X, 0).GetSafeNormal();
							}
							SafeMoveUpdatedComponent(SideDelta, PawnRotation, true, Hit);
						}

						if (bDitch || IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit) || Hit.Time == 0.f)
						{
							remainingTime = 0.f;
							ProcessLanded(Hit, remainingTime, Iterations);
							return;
						}
						else if (GetPerchRadiusThreshold() > 0.f && Hit.Time == 1.f && OldHitImpactNormal.Z >= GetWalkableFloorZ())
						{
							// We might be in a virtual 'ditch' within our perch radius. This is rare.
							const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
							const float ZMovedDist = FMath::Abs(PawnLocation.Z - OldLocation.Z);
							const float MovedDist2DSq = (PawnLocation - OldLocation).SizeSquared2D();
							if (ZMovedDist <= 0.2f * timeTick && MovedDist2DSq <= 4.f * timeTick)
							{
								Velocity.X += 0.25f * GetMaxSpeed() * (RandomStream.FRand() - 0.5f);
								Velocity.Y += 0.25f * GetMaxSpeed() * (RandomStream.FRand() - 0.5f);
								Velocity.Z = FMath::Max<float>(JumpZVelocity * 0.25f, 1.f);
								Delta = Velocity * timeTick;
								SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
							}
						}
					}
				}
			}
		}

		if (Velocity.SizeSquared2D() <= KINDA_SMALL_NUMBER * 10.f)
		{
			Velocity.X = 0.f;
			Velocity.Y = 0.f;
		}

		
	}

}
#pragma endregion
