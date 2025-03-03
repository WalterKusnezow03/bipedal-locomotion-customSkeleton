// Fill out your copyright notice in the Description page of Project Settings.


#include "p2/entities/customIk/bonePackage/BoneController.h"
#include "p2/entities/customIk/bonePackage/TwoBone.h"
#include "p2/entities/customIk/animation/KeyFrameAnimation.h"
#include "p2/entities/customIk/MMatrix.h"
#include "Kismet/KismetMathLibrary.h"
#include "p2/entities/customIk/bonePackage/BoneControllerStates.h"
#include "p2/entities/customIk/animation/motionChain/MotionQueue.h"
#include "p2/entities/customIk/animation/motionChain/MotionAction.h"
#include "CoreMinimal.h"


BoneController::BoneController()
{
	attachedTorso = nullptr;
	attachedCarriedItem = nullptr;
	

	currentMotionState = BoneControllerStates::none; //by default

	//default scales
	fingerScaleCmSetup = 6;
	legScaleCM = 100.0f;
	armScaleCM = 70.0f;

	ALIGNHIP_FLAG = false;

	setupBones();
	setupAnimation(); 
}

BoneController::BoneController(float legScaleCmIn, float armScaleCmIn, int fingerScaleCmIn){
	attachedTorso = nullptr;
	attachedCarriedItem = nullptr;
	legScaleCM = std::abs(legScaleCmIn);
	armScaleCM = std::abs(armScaleCmIn);
	fingerScaleCmSetup = std::abs(fingerScaleCmIn);

	setupBones();
	setupAnimation();
	currentMotionState = BoneControllerStates::none;

	ALIGNHIP_FLAG = false;
}

BoneController::BoneController(BoneController &other){
	attachedCarriedItem = nullptr;
	attachedTorso = nullptr;
	if (&other != this){
		*this = other;
	}

	ALIGNHIP_FLAG = false;
}

BoneController &BoneController::operator=(BoneController &other){
	if(&other == this){
		return *this;
	}

	ownLocation = other.ownLocation;
	ownOrientation = other.ownOrientation;

	ownLocationFoot1 = other.ownLocationFoot1; //real world location end foot 1
	ownLocationFoot2 = other.ownLocationFoot2; //real world location end foot 2

	ownLocationHand1 = other.ownLocationHand1;
	ownLocationHand2 = other.ownLocationHand2;

	hip1MatrixOffset = other.hip1MatrixOffset; //local offset to hip com
	hip2MatrixOffset = other.hip2MatrixOffset; //local offset to hip com

	shoulder1MatrixOffset = other.shoulder1MatrixOffset;
	shoulder2MatrixOffset = other.shoulder2MatrixOffset;

	legDoubleKeys_1 = other.legDoubleKeys_1;
	legDoubleKeys_2 = other.legDoubleKeys_2;

	legScaleCM = other.legScaleCM;

	leg1 = other.leg1;
	leg2 = other.leg2;

	arm1 = other.arm1;
	arm2 = other.arm2;

	armClimbKeys_1 = other.armClimbKeys_1;

	currentMotionState = other.currentMotionState;

	ALIGNHIP_FLAG = false;

	hand1 = other.hand1;
	hand2 = other.hand2;

	//caution: DO NOT COPY MESHES OR ATTACHED ITEMS! (?) -> will not be needed i guess.

	return *this;
}

BoneController::~BoneController()
{

}



int BoneController::armScale(){
	return armScaleCM;
}
int BoneController::legScale(){
	return legScaleCM;
}
int BoneController::fingerScale(HandBoneIndexEnum type){
	return hand1.fingerScale(type);
}

/// @brief gets the look dir of the own orientation matrix
/// @return 
FVector BoneController::lookDirection(){
	return ownOrientation.lookDirXForward().GetSafeNormal();
}


/// @brief finds the bone by index, returns a pointer to the bone
/// @param limbIndex 
/// @return 
TwoBone *BoneController::findBone(int limbIndex){
	if(limbIndex == FOOT_1){
		return &leg1;
	}
	if(limbIndex == FOOT_2){
		return &leg2;
	}
	if(limbIndex == SHOULDER_1){
		return &arm1;
	}
	if(limbIndex == SHOULDER_2){
		return &arm2;
	}

	return nullptr;
}

/// @brief finds the end effector by index, returns a pointer to the endeffector
/// @param limbIndex 
/// @return 
MMatrix *BoneController::findEndEffector(int limbIndex){
	if(limbIndex == FOOT_1){
		return &ownLocationFoot1;
	}
	if(limbIndex == FOOT_2){
		return &ownLocationFoot2;
	}
	if(limbIndex == SHOULDER_1){
		return &ownLocationHand1;
	}
	if(limbIndex == SHOULDER_2){
		return &ownLocationHand2;
	}

	return nullptr;
}



FColor BoneController::limbColor(int limb){
	if(limb == FOOT_1){
		return leg1Color;
	}
	if(limb == FOOT_2){
		return leg2Color;
	}
	if(limb == SHOULDER_1){
		return arm1Color;
	}
	if(limb == SHOULDER_2){
		return arm2Color;
	}
	return FColor::Black;
}



//construct bones
void BoneController::setupBones(){
	/**
     * x is forward, default look dir is foward X,
     * y is right side by default! remember!
     */


	//setup
    leg1.setupBones(legScaleCM);
	leg2.setupBones(legScaleCM);

	arm1.setupBones(armScaleCM);
	arm2.setupBones(armScaleCM);
	arm1.setAsArm();
	arm2.setAsArm();


	int hipSideoffset = 10;
	int shoulderSideOffset = 15;

	//X axis is forward
    FVector sideLeft(0, -hipSideoffset, 0);
    FVector sideRight(0, hipSideoffset, 0);
    hip1MatrixOffset.setTranslation(sideLeft);
    hip2MatrixOffset.setTranslation(sideRight);



	FVector sideLeftShoulder(0, -shoulderSideOffset, armScaleCM);
	FVector sideRightShoulder(0, shoulderSideOffset, armScaleCM);
	shoulder1MatrixOffset.setTranslation(sideLeftShoulder);
	shoulder2MatrixOffset.setTranslation(sideRightShoulder);

	FVector headOffset(0, 0, armScaleCM);
	headMatrixOffset.setTranslation(headOffset);

	FVector weaponSocketFront(armScaleCM * 0.8f, 0.0f, armScaleCM); // same height as arm, forward is X
	weaponMatrixOffset.setTranslation(weaponSocketFront);


	//setup hands
	HandController new1(HandBoneIndexEnum::leftHand, fingerScaleCmSetup);
	HandController new2(HandBoneIndexEnum::rightHand, fingerScaleCmSetup);
	hand1 = new1;
	hand2 = new2;
}


void BoneController::setupAnimation(){
    KeyFrameAnimation legAnimationKeys = KeyFrameAnimation(false); //instant flip animation
	legAnimationKeys.addFrame(
		FVector(0, 0, -legScaleCM),
		0.0f, //time to prev frame
		false,
		legScaleCM
	);
	legAnimationKeys.addFrame(
		FVector(25, 0, -legScaleCM * 0.9),
		0.5f,
		false,
		legScaleCM
	);
	legAnimationKeys.addFrame(
		FVector(50, 0, -legScaleCM),
		0.5f,
		true, //GROUNDED
		legScaleCM
	);

	KeyFrameAnimation legKeysCopy = legAnimationKeys;

	//new
	legSimpleKeys_1 = legAnimationKeys;
	legSimpleKeys_2 = legAnimationKeys;

	//before
	legDoubleKeys_1.setAnimationA(MoveTemp(legAnimationKeys));
	legDoubleKeys_2.setAnimationA(MoveTemp(legKeysCopy));

	FVector hipTargetFromFoot = FVector(0, 0, legScaleCM);
	legDoubleKeys_1.setAnimationBAdjustPermanentTarget(hipTargetFromFoot); //very important!
	legDoubleKeys_2.setAnimationBAdjustPermanentTarget(hipTargetFromFoot);


	//set aniatmion running true or false, 
	legDoubleKeys_1.setRunning(isRunning); //member var from this
	legDoubleKeys_2.setRunning(isRunning);

	//testing - works
	legDoubleKeys_1.scaleToVelocityInCms(100.0f);
	legDoubleKeys_2.scaleToVelocityInCms(100.0f);


	/**
	 * inplace walk setup!
	 */
	//inplace walk
	legInPlaceWalk = KeyFrameAnimation(false); //dont flip!
	legInPlaceWalk.addFrame(
		FVector(0, 0, -legScaleCM),
		0.0f, //time to prev frame
		false,
		legScaleCM
	);
	legInPlaceWalk.addFrame(
		FVector(0, 0, -legScaleCM * 0.8),
		0.2f,
		false,
		legScaleCM
	);
	legInPlaceWalk.addFrame(
		FVector(0, 0, -legScaleCM),
		0.2f,
		true, //GROUNDED
		legScaleCM
	);





	//setup climbing
	KeyFrameAnimation climbAnimationKeys = KeyFrameAnimation(false); //instant flip animation
	climbAnimationKeys.addFrame(
		FVector(0, 0, -armScaleCM),
		0.0f, //time to prev frame
		false,
		armScaleCM
	);
	climbAnimationKeys.addFrame(
		FVector(10, 0, -armScaleCM),
		0.5f, //time to prev frame
		false,
		armScaleCM
	);
	climbAnimationKeys.addFrame(
		FVector(armScaleCM, 0, armScaleCM), //nach oben strecken, so weit wie es geht
		1.5f, //time to prev frame
		true, //GROUNDED
		armScaleCM
	);
	armClimbKeys_1.setAnimationA(MoveTemp(climbAnimationKeys));
	FVector hipTargetFromHand = FVector(0, 0, armScaleCM); //von hand yu hip, was ist der default target wert
	armClimbKeys_1.setAnimationBAdjustPermanentTarget(hipTargetFromHand);

	armClimbKeys_1.setRunning(false);




	//animation states new for motion queue state machine thingy
	MotionAction aimingState;
	FRotator rotationForTarget;
	FVector targetAim(armScaleCM * 0.8f, 0, armScaleCM); //old
	//debug:
	//FVector targetAim(armScaleCM * 0.8f, armScaleCM * 0.5f, armScaleCM);
	aimingState.setLocationAndRotation(targetAim, rotationForTarget);
	armMotionQueue.addTarget(ArmMotionStates::handsFollowItem, aimingState);


	//other states will follow of here for a holstered weapong for example
	MotionAction contactState;
	FRotator rotationForTarget2;
	rotationForTarget2.Pitch = 45; //45 degree to front
	FVector targetContactStateLocation(armScaleCM * 0.5f, 0, armScaleCM);
	contactState.setLocationAndRotation(targetContactStateLocation, rotationForTarget2);
	armMotionQueue.addTarget(ArmMotionStates::kontaktStellung, contactState);

	

	MotionAction holsterState;
	FRotator rotationForTarget3;
	rotationForTarget2.Pitch = -90; //45 degree to front
	FVector targetHolsterStateLocation(0, armScaleCM * 0.1f, 0);
	contactState.setLocationAndRotation(targetHolsterStateLocation, rotationForTarget3);
	armMotionQueue.addTarget(ArmMotionStates::holsterItem, holsterState);


	
}


/// @brief attach limb meshes: top and bottom part of bone
/// @param top 
/// @param bottom 
/// @param index 
void BoneController::attachLimbMeshes(AActor *top, AActor *bottom, int index){
	TwoBone *bone = findBone(index);
	if(bone != nullptr){
		if(top != nullptr){
			bone->attachFirtsLimb(*top);
			top->SetActorEnableCollision(false);
		}
		if(bottom != nullptr){
			bone->attachSecondLimb(*bottom);
			bottom->SetActorEnableCollision(false);
		}	
	}
}


void BoneController::attachPedalFoots(AActor *left, AActor *right){
	TwoBone *leftBone = findBone(FOOT_1);
	if(leftBone != nullptr && left != nullptr){
		left->SetActorEnableCollision(false);
		leftBone->attachThirdLimb(*left);
	}
	TwoBone *rightBone = findBone(FOOT_2);
	if(rightBone != nullptr && right != nullptr){
		right->SetActorEnableCollision(false);
		rightBone->attachThirdLimb(*right);
	}
}


void BoneController::attachTorso(AActor *torsoPointerIn){
	if(torsoPointerIn != nullptr){
		attachedTorso = torsoPointerIn;
	}
}

void BoneController::attachHead(AActor *headPointer){
	if(headPointer != nullptr){
		attachedHead = headPointer;
	}
}

void BoneController::attachFinger(
	AActor *top, 
	AActor *bottom, 
	HandBoneIndexEnum hand,
	HandBoneIndexEnum finger
){
	if(hand == HandBoneIndexEnum::leftHand){
		hand1.attachLimbMeshes(top, bottom, finger);
	}
	if(hand == HandBoneIndexEnum::rightHand){
		hand2.attachLimbMeshes(top, bottom, finger);
	}
}






void BoneController::drawBody(float DeltaTime){
	return;

	MMatrix current = currentTransform();
	MMatrix shoulder1 = currentTransform(SHOULDER_1);
	MMatrix shoulder2 = currentTransform(SHOULDER_2);
	MMatrix foot1 = currentTransform(FOOT_1);
	MMatrix foot2 = currentTransform(FOOT_2);

	FVector com = current.getTranslation();

	DeltaTime *= 1.1f;

	/*
	DebugHelper::showLineBetween(GetWorld(), com, shoulder1.getTranslation(), FColor::Red, DeltaTime);
	DebugHelper::showLineBetween(GetWorld(), com, shoulder2.getTranslation(), FColor::Red, DeltaTime);
	DebugHelper::showLineBetween(GetWorld(), com, foot1.getTranslation(), FColor::Red, DeltaTime);
	DebugHelper::showLineBetween(GetWorld(), com, foot2.getTranslation(), FColor::Red, DeltaTime);
	*/
}


/// @brief will update the torso location 
void BoneController::TickUpdateTorso(){
	if(attachedTorso != nullptr){
		MMatrix currentTransformMat = currentTransform();
		FRotator orient = ownOrientation.extractRotator();
		attachedTorso->SetActorLocation(currentTransformMat.getTranslation());
		attachedTorso->SetActorRotation(orient);
	}
}

/// @brief will update the head location (for now, individual rotation will be added later.)
void BoneController::TickUpdateHead(){
	if(attachedHead != nullptr){
		MMatrix currentTransformMat = currentTransform(HEAD);
		FRotator orient = ownOrientation.extractRotator();
		attachedHead->SetActorLocation(currentTransformMat.getTranslation());
		attachedHead->SetActorRotation(orient);
	}
}




void BoneController::SetLocation(FVector &pos){
    //setup
    ownLocation.setTranslation(pos);
	refreshLocomotionframes();
}

FVector BoneController::GetLocation(){
	return currentTransform().getTranslation();
}


bool BoneController::isANewLookDirection(FVector &other){
	other = other.GetSafeNormal();
	if (FVector::DotProduct(other, latestLookAtDirection) >= 1.0f){
		return false;
	}
	latestLookAtDirection = other;
	return true;
}

/// @brief is not tested!
/// @param TargetLocation 
void BoneController::LookAt(FVector TargetLocation) 
{
	if(ALIGNHIP_FLAG){
		return;
	}

	//skalar product prüfen ob lookdir schon gegeben
	FVector connect = TargetLocation - ownLocation.getTranslation();
	connect = connect.GetSafeNormal();

	if(isANewLookDirection(connect)){
		FVector forward = ownOrientation.lookDirXForward().GetSafeNormal();
		float dotProduct = FVector::DotProduct(forward, connect);

		if(dotProduct >= 0.99f){
			ALIGNHIP_FLAG = false;
			
			// DebugHelper::showScreenMessage("ROTATION ALREADY REACHED!");
			return;
		}
		//isWaitingForAnimStop = true;


		//walk towards wanted rotation
		if(currentMotionState == BoneControllerStates::none){
			rotationWithoutLocomotion = true;
			currentMotionState = BoneControllerStates::locomotion;
		}
	
		if(currentMotionState == BoneControllerStates::locomotion){
			latestLookAtDirection = connect;

			float flip = forward.X * connect.Y - forward.Y * connect.X < 0.0f ? -1.0f : 1.0f;
			float signedAngle = flip * MMatrix::radToDegree(std::acosf(dotProduct));
			updateRotation(signedAngle);
		}
	}
}

void BoneController::updateRotation(float signedAngle){
	if(ALIGNHIP_FLAG == false){
		if(leg1isPlaying && legDoubleKeys_1.isAnimationA()){
			legDoubleKeys_1.rotateNextFramesA(signedAngle);
		}
		if(!leg1isPlaying && legDoubleKeys_2.isAnimationA()){
			legDoubleKeys_2.rotateNextFramesA(signedAngle);
		}
		ALIGNHIP_FLAG = true;
		lookAtPendingAngle = signedAngle;

		//DebugHelper::showScreenMessage("new rotation", lookAtPendingAngle);
	}
}



void BoneController::resetPendingRotationStatus(){
	ALIGNHIP_FLAG = false;
	lookAtPendingAngle = 0.0f;

	//reset rotation locotmotion flag
	if(rotationWithoutLocomotion){
		currentMotionState = BoneControllerStates::none;
		rotationWithoutLocomotion = false;
	}
}




void BoneController::overrideRotationYaw(float degree){
	ownOrientation.resetRotation();
	ownOrientation.yawRadAdd(MMatrix::degToRadian(degree));
}

/// @brief method to attach a carried item to the hands of the actor, will override the current
/// item if not nullptr
/// @param carriedItem 
void BoneController::attachCarriedItem(AcarriedItem *carriedItem){
	if(carriedItem != nullptr){
		attachedCarriedItem = carriedItem;
		attachedCarriedItem->SetActorEnableCollision(false);
		weaponAimDownSight();
	}
}




MMatrix BoneController::currentFootTransform(MMatrix &foottranslationToRotate){
	//M = T * R <-- lese richtung --
	MMatrix rotationTransform = foottranslationToRotate * ownOrientation; //rotiert fuss das es übereinstimmt mit actor
	return rotationTransform;
}



/// @brief returns the current translation and rotation in world in correct order as expected
/// HIP PIVOT TRANSFORM ONLY
/// @return current Transform Matrix by value
MMatrix BoneController::currentTransform(){ //might be renamed to hip pivot 
	MMatrix rotationTransform = ownLocation * ownOrientation;
	return rotationTransform;
}


MMatrix BoneController::currentTransform(MMatrix &offset){
	//M = T * R * Toffset <-- lese richtung --
    MMatrix RT = ownOrientation * offset; //erst schulter offset z.b.
    MMatrix rotationTransform = ownLocation * RT; //rotiert fuss das es übereinstimmt mit actor
    return rotationTransform;
}


/// @brief gets the current transform for the relative starting joint by limb index
/// @param leg limb index (FOOT_1 for example, or HEAD)
/// @return MMatrix current transform
MMatrix BoneController::currentTransform(int leg){ 
    if(leg == FOOT_1){
		return currentTransform(hip1MatrixOffset);
    }
    if(leg == FOOT_2){
        return currentTransform(hip2MatrixOffset);
    }
	if(leg == SHOULDER_1){
		return currentTransform(shoulder1MatrixOffset);
    }
    if(leg == SHOULDER_2){
        return currentTransform(shoulder2MatrixOffset);
    }
	if(leg == HEAD){
		return currentTransform(headMatrixOffset);
	}

	return currentTransform();
}

MMatrix BoneController::offsetMatrixByLimb(int limb){
	if(limb == FOOT_1){
		return hip1MatrixOffset;
    }
    if(limb == FOOT_2){
        return hip2MatrixOffset;
    }
	if(limb == SHOULDER_1){
		return shoulder1MatrixOffset;
    }
    if(limb == SHOULDER_2){
        return shoulder2MatrixOffset;
    }
	return MMatrix();
}

/// @brief creates the inverse matrix to remove from a position 
/// to create the hip position
/// @param limb 
/// @return inverse matrix to remove the rotated limb offset
MMatrix BoneController::offsetInverseMatrixByLimb(int limb){
	MMatrix offsetLimb = offsetMatrixByLimb(limb); // T
	MMatrix offset = ownOrientation * offsetLimb; // M = R * T <-- lese richtung (erst offset) --
	
	return offset.createInverse();
}



/// @brief converts a position to relative to the hip / actor center
/// @param position position to convert, will be adjusted by reference
void BoneController::transformFromWorldToLocalCoordinates(FVector &position, int limb){

	//M = C * B * A
	/*
	MMatrix A = offsetMatrixByLimb(limb).createInverse();
	MMatrix B = ownOrientation.createInverse();
	MMatrix C = ownLocation.createInverse();

	MMatrix inverse = C * B;
	inverse *= A;
	*/
	

	// testing (scheint zu funktionieren)
	MMatrix inverse = currentTransform(limb).jordanInverse();

	position = inverse * position;
}






/// @brief update hip location relative to some leg / bone starting joint 
/// (The inverse will be taken and position of hip / root adjusted accordingly)
/// (shoulder / hip offset)
/// @param updatetHipJointMat updated matrix (relative root transform, for example shoulder) 
/// @param limbindex limb index of the limb 
void BoneController::updateHipLocation(MMatrix &updatetHipJointMat, int limbindex){
    
	//mit inverse die matrix zurück rechnen und root setzen.
	
	//offset invertieren
	MMatrix offset = offsetInverseMatrixByLimb(limbindex);
	updatetHipJointMat *= offset; //M = T * back  <-- lese richtung --
	ownLocation = updatetHipJointMat;
}


/**
 * testing needed!
 */
void BoneController::updateHipLocationAndRotation(MMatrix &updatedStartingJointMat, int limbindex){
	MMatrix offsetRaw = offsetInverseMatrixByLimb(limbindex);
	MMatrix rotation = updatedStartingJointMat.extarctRotatorMatrix();
	offsetRaw *= rotation; //M = R * T <--lese richtung --

	updatedStartingJointMat *= offsetRaw;
	ownLocation.setTranslation(updatedStartingJointMat);

	ownOrientation.setRotation(rotation);
}

bool BoneController::canChangeStateNow(){
	return currentMotionState != BoneControllerStates::locomotionClimbAll;
}


//api for updating states easily
void BoneController::setStateWalking(){
	if(canChangeStateNow()){
		if(currentMotionState != BoneControllerStates::locomotion){
			currentMotionState = BoneControllerStates::locomotion;
			isRunning = false;
			refreshLocomotionframes();
		}
		
	}
}

void BoneController::setStateRunning(){
	if(canChangeStateNow()){
		currentMotionState = BoneControllerStates::locomotion;
		isRunning = true;

		refreshLocomotionframes();
	}
}

void BoneController::refreshLocomotionframes(){
	if(currentMotionState == BoneControllerStates::locomotion){
		if(leg1isPlaying){

			FrameProjectContainer container = generateFrameProjectContainer(FOOT_1);
			legDoubleKeys_1.forceRefreshTarget(container);

		}else{
			FrameProjectContainer container = generateFrameProjectContainer(FOOT_2);
			legDoubleKeys_2.forceRefreshTarget(container);
			
		}
	}
}




void BoneController::stopLocomotion(){
	if(currentMotionState != BoneControllerStates::none){
		isWaitingForAnimStop = true;
	}
	
}





/// @brief needs to be called every frame from the player!
/// @param camera camera of the player
void BoneController::updateStatesBasedOnCamera(UCameraComponent &camera){
	/**
	 * IS TESTED - should ensure proper weapon location infront of camera
	 */
	if(canChangeStateNow()){

		FVector camForward = camera.GetForwardVector();
		FVector camLocation = camera.GetComponentLocation() +
							  camForward * armScaleCM * 0.95f;

		//location ins locale bringen!
		currentTransform().transformFromWorldToLocalCoordinates(camLocation);

		FRotator cameraRot = camera.GetComponentRotation();


		FRotator camPitched;
		camPitched.Pitch = cameraRot.Pitch * -1.0f; //must be flipped.

		
		//ADS
		MotionAction action;
		action.setLocationAndRotation(camLocation, camPitched); //local matrix now
		armMotionQueue.addTarget(ArmMotionStates::handsFollowItem, action);


		//HIP
		camLocation += FVector(-armScaleCM * 0.2f,0,0) + 
					  FVector(0, armScaleCM * 0.2f, 0);
		MotionAction hipaction;
		hipaction.setLocationAndRotation(camLocation, camPitched); //local matrix now
		armMotionQueue.addTarget(ArmMotionStates::kontaktStellung, hipaction);
	}
}


void BoneController::weaponAimDownSight(bool aimStatus){
	if(aimStatus){
		weaponAimDownSight();
	}else{
		weaponContactPosition();
	}
}

void BoneController::weaponAimDownSight(){
	//block arm motion state changes while climbing
	if(canChangeStateNow()){
		if(attachedCarriedItem != nullptr){
			armMotionQueue.updateStateIfPossible(ArmMotionStates::handsFollowItem);
		}
	}
}

void BoneController::weaponContactPosition(){
	//block arm motion state changes while climbing
	if(canChangeStateNow()){
		if(attachedCarriedItem != nullptr){
			armMotionQueue.updateStateIfPossible(ArmMotionStates::kontaktStellung);
		}
	}
}

void BoneController::weaponHolsterPosition(){
	if(canChangeStateNow()){
		if(attachedCarriedItem != nullptr){
			armMotionQueue.updateStateIfPossible(ArmMotionStates::holsterItem);
		}
	}
}



void BoneController::dropWeapon(){
	attachedCarriedItem = nullptr;
}

//will be designed for arms
void BoneController::Tick(float DeltaTime, UWorld *worldIn, FVector overrideLocation){
	ownLocation.setTranslation(overrideLocation);
	Tick(DeltaTime, worldIn);
}

/// @brief must be ticked from outside skelletal owner
///
/// @param DeltaTime 
/// @param actorLocation ---> new location for this bone controller, COPIED MATRIX!
/// @param world ---> world is passed AND SAVED!, caution 
void BoneController::Tick(float DeltaTime, UWorld *worldIn){
    world = worldIn;

	drawBody(DeltaTime); //debug draw body
	TickUpdateTorso();
	TickUpdateHead();

	if(currentMotionState == BoneControllerStates::none){
        TickBuildNone(DeltaTime);
    }
    if(currentMotionState == BoneControllerStates::locomotion){
		//DebugHelper::showScreenMessage("locomotion!", FColor::Red);
		TickLocomotion(DeltaTime);
		TickArms(DeltaTime);
	}
	if(currentMotionState == BoneControllerStates::locomotionClimbAll){
		TickLocomotionClimbAll(DeltaTime);
	}

}



void BoneController::waitForLocomotionStopIfNeeded(){
	if(isWaitingForAnimStop){
		currentMotionState = BoneControllerStates::none;
		isWaitingForAnimStop = false;
	}
}






/// @brief Tick Locomotion Based on State 
/// @param DeltaTime 
void BoneController::TickLocomotion(float DeltaTime){
	//block motion auto adjust for new motion queue doing the job
	if(ALIGNHIP_FLAG){
		if(legDoubleKeys_1.isAnimationB() || legDoubleKeys_2.isAnimationB()){
			TickHipAutoAlign(DeltaTime);
			return;
		}
	}


	//default player
	if (leg1isPlaying)
	{
		buildRawAndKeepEndInPlace(leg2, ownLocationFoot2, DeltaTime, leg2Color, FOOT_2);
		playForwardAndBackwardKinematicAnim(leg1, legDoubleKeys_1, ownLocationFoot1, DeltaTime, leg1Color, FOOT_1);

		//switch to next leg but update relative position
		if(legDoubleKeys_1.animationCycleWasComplete()){
			leg1isPlaying = false;

			//die hip wurde mit dem anderen bein bewegt /(leg 1, also muss leg2 geupdated werden)
			FVector footPos = ownLocationFoot2.getTranslation();
			//DebugHelper::showLineBetween(world, footPos, currentTransform(FOOT_2).getTranslation(), FColor::Orange, 2.0f);
			transformFromWorldToLocalCoordinates(footPos, FOOT_2);
			legDoubleKeys_2.overrideCurrentStartingFrame(footPos); //update starting pos of the foot because the actor moved
	
			//stop anim if finished
			waitForLocomotionStopIfNeeded();
		}
	}else{
		
		buildRawAndKeepEndInPlace(leg1, ownLocationFoot1, DeltaTime, leg1Color, FOOT_1);
		playForwardAndBackwardKinematicAnim(leg2, legDoubleKeys_2, ownLocationFoot2, DeltaTime, leg2Color, FOOT_2);

		if(legDoubleKeys_2.animationCycleWasComplete()){//left leg done
			leg1isPlaying = true;

			//die hip wurde mit dem anderen bein bewegt /(leg 2, also muss leg1 geupdated werden)
			FVector footPos = ownLocationFoot1.getTranslation();
			//DebugHelper::showLineBetween(world, footPos, currentTransform(FOOT_1).getTranslation(), FColor::Orange, 2.0f);
			
            transformFromWorldToLocalCoordinates(footPos, FOOT_1);
			legDoubleKeys_1.overrideCurrentStartingFrame(footPos); //update starting pos of the foot because the actor moved

			//stop anim if finished
			waitForLocomotionStopIfNeeded();
		}
	}


	
}


//leg only for now!
void BoneController::TickHipAutoAlign(float DeltaTime){

	if(!ALIGNHIP_FLAG){
		return;
	}

	//tick legs
	buildRawAndKeepEndInPlace(leg1, ownLocationFoot1, DeltaTime, leg1Color, FOOT_1);
	buildRawAndKeepEndInPlace(leg2, ownLocationFoot2, DeltaTime, leg2Color, FOOT_2);
	

	//build
	int index = leg1isPlaying ? FOOT_1 : FOOT_2;
	MMatrix *end = findEndEffector(index);
	TwoBone *bone = findBone(index);

	if(end != nullptr && bone != nullptr){

		float averageVelocityOfAnimation = 0.0f;
		FVector garivityVec;
		if(leg1isPlaying){
			garivityVec = legDoubleKeys_1.getProjectionOffsetTimed(DeltaTime, end->getTranslation());
			averageVelocityOfAnimation = legDoubleKeys_1.averageVelocity();
		}
		else
		{
			garivityVec = legDoubleKeys_2.getProjectionOffsetTimed(DeltaTime, end->getTranslation());
			averageVelocityOfAnimation = legDoubleKeys_2.averageVelocity();
		}

		//operator is overloaded
		*end += garivityVec;
		ownLocation += garivityVec;

		MMatrix jointStart = currentTransform(index);


		bool reachedHipTargetAutoAdjust = false;
		MMatrix update = legMotionQueue.TickUpdatedHipMoveAlignMatrix(
			jointStart,
			ownOrientation,
			*end,
			*bone,
			DeltaTime,
			lookAtPendingAngle,
			GetWorld(),
			reachedHipTargetAutoAdjust,
			//200.0f //2m/s
			averageVelocityOfAnimation 
		);

		if(!reachedHipTargetAutoAdjust)
			updateHipLocationAndRotation(update, index);

		if(reachedHipTargetAutoAdjust){

			//DebugHelper::showScreenMessage("new rotation reached");

			//stop anim.
			//ALIGNHIP_FLAG = false;
			resetPendingRotationStatus();

			legDoubleKeys_1.resetAnimationToStartAndResetRotation();
			legDoubleKeys_2.resetAnimationToStartAndResetRotation();


			//das hier auszukommentieren, bringt nichts beim bug wo die end limbs so rumglitchen
			//auf 90 grad dings.
			//leg 1 reached, 2 update just as default walking
			if(leg1isPlaying){
				FVector footPos = ownLocationFoot2.getTranslation();
				transformFromWorldToLocalCoordinates(footPos, FOOT_2);
				legDoubleKeys_2.overrideCurrentStartingFrame(footPos);
			}else{
				FVector footPos = ownLocationFoot1.getTranslation();
				transformFromWorldToLocalCoordinates(footPos, FOOT_1);
				legDoubleKeys_1.overrideCurrentStartingFrame(footPos);
			}

			leg1isPlaying = !leg1isPlaying;
			return;
		}
	}

	
}







/// @brief will tick the arms and legs no motion
/// @param DeltaTime delta time from last tick
void BoneController::TickBuildNone(float DeltaTime){
	
    //hier müssen alle bones einfach gebaut werden
	TickArms(DeltaTime);
	TickLegsNone(DeltaTime);
}


/// @brief will tick the arm movement if not climbing
/// call this method for any arm related tick
/// @param DeltaTime 
void BoneController::TickArms(float DeltaTime){

	//move arms if not climbing and no transition is left
	if(currentMotionState == BoneControllerStates::locomotionClimbAll){
		if(!armMotionQueue.isTransitioning()){ //no transition is left
			return;
		}
	}


	MMatrix *endEffectorLeft = findEndEffector(SHOULDER_1);
	MMatrix *endEffectorRight = findEndEffector(SHOULDER_2);

	TwoBone *leftArm = findBone(SHOULDER_1);
	TwoBone *rightArm = findBone(SHOULDER_2);
	if(
		rightArm != nullptr && leftArm != nullptr &&
		endEffectorRight != nullptr && endEffectorLeft != nullptr
	){

		MMatrix transform = currentTransform();
		MMatrix transformLeftArm = currentTransform(SHOULDER_1);
		MMatrix transformRightArm = currentTransform(SHOULDER_2);


		armMotionQueue.Tick(
			transform, 
			transformLeftArm,
			transformRightArm,
			*endEffectorRight,
			*endEffectorLeft,
			*leftArm, 
			*rightArm, 
			hand1, //left hand new
			hand2, //right hand new
			attachedCarriedItem, 
			world,
			DeltaTime
		);
				
	}
	
}






void BoneController::moveBoneAndSnapEndEffectorToTarget(
	int index, 
	float DeltaTime, 
	FVector targetWorld,
	FVector weight
){
	weight = weight.GetSafeNormal();

	TwoBone *bone = findBone(index);
	MMatrix *end = findEndEffector(index);
	MMatrix translationActor = currentTransform(index);
	FColor color = limbColor(index);

	if (bone != nullptr && end != nullptr)
	{

		FVector targetLocal = targetWorld;
		transformFromWorldToLocalCoordinates(targetLocal, index);
		
		bone->rotateEndToTargetAndBuild(
			GetWorld(),
			targetLocal,
			weight,
			translationActor, // hip start with orient
			*end, //ownLocationFoot,  // foot apply positions
			color, 
			DeltaTime * 2.0f
		);
	}
}




/// @brief build both legs, no motion
/// @param DeltaTime 
void BoneController::TickLegsNone(float DeltaTime){
	TickLimbNone(FOOT_1, DeltaTime);
	TickLimbNone(FOOT_2, DeltaTime);
}

void BoneController::TickLimbNone(int limbIndex, float DeltaTime){
	
	TwoBone *bone = findBone(limbIndex);
	MMatrix *endEffector = findEndEffector(limbIndex);

	if(bone != nullptr && endEffector != nullptr){

		MMatrix transform = currentTransform(limbIndex);

		bone->build(
			GetWorld(),
			transform,
			*endEffector, // by reference updated
			limbColor(limbIndex),
			DeltaTime * 2.0f
		);
	}
}

/**
 * 
 * ----- ARM FORWARD IK SECTION ------
 * 
 */



void BoneController::playForwardKinematicAnim(
	KeyFrameAnimation &frames, 
	float DeltaTime,
	int limbIndex
){

	TwoBone *bone = findBone(limbIndex);
	MMatrix *endEffector = findEndEffector(limbIndex);
	if(bone != nullptr && endEffector != nullptr){
		MMatrix initalTransform = currentTransform(limbIndex);
		FColor color = limbColor(limbIndex);

		//testing
		FVector weight(1, 0, 0); //x is forward
	
		//FVector weight(-1, 1, 0); //testing needed for arm rotation
		FVector nextPos = frames.interpolate(DeltaTime); //new function
		
		bone->rotateEndToTargetAndBuild(
			GetWorld(),
			nextPos,
			weight,
			initalTransform, //offset and rotation
			*endEffector, //end effector to update
			color,
			DeltaTime * 2.0f
		);
	}

	
}












/**
 * 
 * 
 * ----- HIP SECTION / DOUBLE KEY FRAME ANIMATION ------
 * 
 * 
 */

/// @brief renders the leg raw if the hip is moved!
/// @param boneIk bone to render which foot stays in place
/// @param legEndTransform leg position to hold and not move from
/// @param deltaTime delta time
/// @param color color to render
void BoneController::buildRawAndKeepEndInPlace(
    TwoBone &boneIk, 
    MMatrix &legEndTransform, 
    float deltaTime, 
    FColor color,
    int leg
){
    
    

    /*
	build leg raw

	ziel hier: 
	build sodass von hier das bein am originalen punkt bleibt
	das foot transform muss also zu einem target umgewandelt werden
	und dann dort hin bewegt. Egal ob was neu berechnet wird oder nicht
	*/

	FVector weight(1, 0, 0); // gewicht zieht nach vorne
	
	FVector pos = legEndTransform.getTranslation();
    MMatrix currenttransform = currentTransform(leg);

	//relativ zur hüfte neu berechnen, auchwenn hüfte sich bewegt!
	currenttransform.transformFromWorldToLocalCoordinates(pos); 

	boneIk.rotateEndToTargetAndBuild(
		GetWorld(),
		pos,
		weight,
		currenttransform, // MMatrix &offsetAndRotation,
		legEndTransform,
		color,
		deltaTime * 2.0f // displayTime
	);

}





/**
 * 
 * 
 * FOOT ANIMATION! 
 * 
 * 
 */


void BoneController::playForwardAndBackwardKinematicAnim(
	TwoBone &bone, 
	DoubleKeyFrameAnimation &frames, 
	MMatrix &footMatrix, //MMatrix foot transform 
	float DeltaTime,
	FColor color,
    int leg //bein 1 oder 2 fürs erste, könnte auch liste werden
){
	
	bool moveLeg = frames.isAnimationA();

	if(moveLeg){
        // ---- FORWARD BUILD ----
		playForwardKinematicAnim(bone, frames, footMatrix, DeltaTime, color, leg);	
	}else{
		// ---- HIP ADJUST BUILD ----
		playBackwardKinematicAnim(bone, frames, footMatrix, DeltaTime, color, leg);
	}
}




/// @brief 
/// @param bone 
/// @param frames 
/// @param footMatrix 
/// @param DeltaTime 
/// @param color 
/// @param leg 
void BoneController::playForwardKinematicAnim(
	TwoBone &bone, 
	DoubleKeyFrameAnimation &frames, 
	MMatrix &footMatrix,
	float DeltaTime,
	FColor color,
    int leg
){

	// add velocity and project frames 
    
	

	FrameProjectContainer container = generateFrameProjectContainer(leg);
	frames.projectNextFrameIfNeeded(container);

	FVector thisAdd = container.getLookDir() * container.getVelocity() * DeltaTime;
	ownLocation += thisAdd;


	//override motion state if needed
	//if(switchToClimbLocomotion && currentMotionState == BoneControllerStates::locomotion){
	if(container.startClimb()){
		currentMotionState = BoneControllerStates::locomotionClimbAll; //switch to climb motion

		//put item to holster
		armMotionQueue.updateState(ArmMotionStates::holsterItem);
		return;
	}

	//do movement
	//nochmal neu rechnen nach offset
    MMatrix translationActor = currentTransform(leg);	
	
	FVector weight(1, 0, 0);
	

	//interpolate World is used to play any animation - even if no target is setup!
	FVector nextPos = frames.interpolateWorld(
		DeltaTime,
		footMatrix.getTranslation(),
		translationActor
	);
	


	bone.rotateEndToTargetAndBuild(
		GetWorld(),
		nextPos,
		weight,
		translationActor, // hip start with orient
		footMatrix, //ownLocationFoot,  // foot apply positions
		color, 
		DeltaTime * 2.0f
	);
}


/// @brief plays the animation and adjusts the hip of the actor based on the movement made
/// the end effector will be kept in place
/// @param bone bone to move start effector from (the hip)
/// @param frames frames to play
/// @param footMatrix end effector matrix 
/// @param DeltaTime deltatime
/// @param color color
/// @param leg limb index
void BoneController::playBackwardKinematicAnim(
	TwoBone &bone, 
	DoubleKeyFrameAnimation &frames, 
	MMatrix &footMatrix, //MMatrix foot transform 
	float DeltaTime,
	FColor color,
    int leg
){

	FVector weight(1, 0, 0);

	//offset for foot during hip adjust movement
	
	FVector hipoffsetAdd = frames.getProjectionOffsetTimed(DeltaTime, footMatrix.getTranslation());
	footMatrix += hipoffsetAdd;
	


	//hip matrix aber da wo der fuss attached ist am oberschenkel
    MMatrix ownLocationRelativeToHip = currentTransform(leg);

	//world system vom fuss
	MMatrix current_RotatedFootSystem = currentFootTransform(footMatrix);

	
	//new live update local position at all times
	FVector StartEffectorWorld = ownLocationRelativeToHip.getTranslation(); //hip start
	MMatrix inverse = current_RotatedFootSystem.jordanInverse(); //foot local system
	FVector startEffectorLocal = inverse * StartEffectorWorld; //hip relative to current foot / end effector pos

	//interpolated with live location update
	FVector xt = frames.interpolate(DeltaTime, startEffectorLocal); 
	
	

    
    
    //MMatrix current_translationActorFoot = currentFootTransform(footMatrix);
	bone.rotateStartToTargetAndBuild(
		GetWorld(),
		xt,
		weight,
		current_RotatedFootSystem, // foot start
		ownLocationRelativeToHip, // hip apply / update der zwischen matrix 
		color, //FColor::Blue,
		DeltaTime * 2.0f
	);

    //inverse berechnen sodass man den offset zurück wandert und hip beinflusst
	//wichtig!
	updateHipLocation(ownLocationRelativeToHip, leg); 


}




UWorld *BoneController::GetWorld(){
    return world;
}



float BoneController::addVelocityBasedOnState(){
	if(
		currentMotionState == BoneControllerStates::locomotion &&
		isRunning
	){
		return 200.0f;
	}

	if(currentMotionState == BoneControllerStates::locomotion){
		return 0.0f; //extra
	}

	return 0.0f;
}














/// @brief will allow the actor to climb. If the arm motion queue is transitioning
/// it will be ticked until its finished (for example to holster the weapon)
/// @param DeltaTime 
void BoneController::TickLocomotionClimbAll(float DeltaTime){
	

	//new: block climbing while arm motion is transitioning
	if(armMotionQueue.isTransitioning()){
		TickLegsNone(DeltaTime);
		TickArms(DeltaTime);
		return;
	}

	//switch to next leg but update relative position
	
	//hand reached target, end effector bleibt gelockt für das bein
	if(armClimbKeys_1.isAnimationB() && !climb_setHandTarget){ 

		climb_setHandTarget = true;
		
		FVector footPos = ownLocationFoot1.getTranslation();
		FVector frame = ownLocationHand1.getTranslation();
		MMatrix startMat = currentTransform(FOOT_1);
		legDoubleKeys_1.skipAnimationOnceWorld(startMat, footPos, frame);
		
	}
	
	//END OF ANIMATION ALL
	//switch to next leg but update relative position
	if(legDoubleKeys_1.isAnimationB() && climb_hand1cycleComplete){
		
		currentMotionState = BoneControllerStates::locomotion;
		climb_setHandTarget = false;
		climb_hand1cycleComplete = false;

		armMotionQueue.updateState(ArmMotionStates::handsFollowItem); //switch to aim state again
		return;
	}
	
	//if climb complete: allow leg to finish
	if(armClimbKeys_1.animationCycleWasComplete()){
		if(climb_hand1cycleComplete == false){
			arm1.resetAllRotations();
		}

		climb_hand1cycleComplete = true;

	}


	//build bones
	std::vector<int> limbs;
	std::vector<DoubleKeyFrameAnimation *> animations;


	//nur anticken wenn man sich hochzieht, und noch nicht fertig war
	if(!climb_hand1cycleComplete){
		limbs.push_back(SHOULDER_1);
		animations.push_back(&armClimbKeys_1);
	}else{
		TickLimbNone(SHOULDER_1, DeltaTime);
	}

	//add leg motion if hand already reached its target
	if(climb_setHandTarget){ 
		limbs.push_back(FOOT_1);
		animations.push_back(&legDoubleKeys_1);
	}
	

	playForwardAndBackwardKinematicAnimSynchronized(
		DeltaTime,
		limbs,
		animations
	);




	// will skip arms which are invloved in climbing
	TickArms(DeltaTime);
	TickLimbNone(FOOT_2, DeltaTime);

	
}




/**
 * plays animations for limbs and their animations,
 * caution: the first bone to play backward kinematic in list, will be the only one for this
 * function call. Prevents bugs. 
 * All other animations will fix them selve because
 * the world-frame is passed and updates the animation dynamically later
 */
void BoneController::playForwardAndBackwardKinematicAnimSynchronized(
	float DeltaTime,
	std::vector<int> limbs,
	std::vector<DoubleKeyFrameAnimation*> animations
){
	bool backwardKinematicIsBlocked = false;
	for (int i = 0; i < limbs.size(); i++){
		if(i < animations.size()){
			int limbIndex = limbs[i];
			DoubleKeyFrameAnimation *frames = animations[i];
			TwoBone *bone = findBone(limbIndex);
			MMatrix *effectorMat = findEndEffector(limbIndex);
			FColor color = limbColor(limbIndex);
			if (
				frames != nullptr &&
				bone != nullptr &&
				effectorMat != nullptr
			){
				bool moveLeg = frames->isAnimationA(); //otherwise is B, backward build, BK 
				if(moveLeg){
					// ---- FORWARD BUILD ----
					playForwardKinematicAnim(*bone, *frames, *effectorMat, DeltaTime, color, limbIndex);	
				}
				if(!moveLeg && !backwardKinematicIsBlocked){
					// ---- HIP ADJUST BUILD ----
					playBackwardKinematicAnim(*bone, *frames, *effectorMat, DeltaTime, color, limbIndex);
					backwardKinematicIsBlocked = true;

					//FString text = FString::Printf(TEXT("blocked backward adjust by  %d"), limbIndex);
					//DebugHelper::logMessage(text);
				}
			}
		}
	}
	
}





/**
 * 
 * new frame project container
 * 
 */
FrameProjectContainer BoneController::generateFrameProjectContainer(int limbindex){
	FrameProjectContainer container;

	MMatrix current = currentTransform(limbindex); //based on leg transform
	float velocityT = addVelocityBasedOnState(); 
    FVector lookDir = currentTransform().lookDirXForward();
	lookDir.Z = 0.0f; //xy pane only of interest

	float minHeightClimb = armScaleCM * 1.8f;
	float maxHeightDoesntAllowClimb = armScaleCM * 2.5f; // max height
	container.setup(
		GetWorld(), 
		current, 
		velocityT, 
		lookDir, 
		minHeightClimb, 
		maxHeightDoesntAllowClimb,
		currentMotionState
	);

	return container;
}






/**
 * new debug
 */
void BoneController::debugUpdateTransform(FVector location, FRotator rotation){
	ownLocation.setTranslation(location);
	ownOrientation.setRotation(rotation);
}


FVector BoneController::stabilizedHipLocation(){
	FVector currentLocation = GetLocation();
	if(currentMotionState == BoneControllerStates::locomotion){
		if(leg1isPlaying){
			FVector ground = legDoubleKeys_1.copyGroundPosition();
			ground += FVector(0, 0, legScaleCM);
			ground.X = currentLocation.X;
			ground.Y = currentLocation.Y;
			return ground;
		}
		else
		{
			FVector ground = legDoubleKeys_2.copyGroundPosition();
			ground += FVector(0, 0, legScaleCM);
			ground.X = currentLocation.X;
			ground.Y = currentLocation.Y;
			return ground;
		}
	}

	return currentLocation;
}

void BoneController::debugDrawHeadForward(UWorld *worldPointer, float DeltaTime){
	MMatrix t = currentTransform(HEAD);
	FVector a = t.getTranslation();
	FVector b = ownOrientation.lookDirXForward() * 100.0f + a;
	DebugHelper::showLineBetween(worldPointer, a, b, FColor::Orange, DeltaTime * 2.0f);
}















/**
 * 
 * 
 * new hand section -> empty yet
 * 
 * 
 */
void BoneController::TickHandsNone(float DeltaTime){

}