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
	setupBones();
	setupAnimation(); // temporary zum denken

	currentMotionState = BoneControllerStates::none; //by default

}

BoneController::BoneController(float legScaleCmIn, float armScaleCmIn){
	attachedTorso = nullptr;
	attachedCarriedItem = nullptr;
	legScaleCM = std::abs(legScaleCmIn);
	armScaleCM = std::abs(armScaleCmIn);
	setupBones();
	setupAnimation();
	currentMotionState = BoneControllerStates::none;
}

BoneController::BoneController(BoneController &other){
	attachedCarriedItem = nullptr;
	attachedTorso = nullptr;
	if (&other != this){
		*this = other;
	}
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

	//caution: DO NOT COPY MESHES OR ATTACHED ITEMS! (?) -> will not be needed i guess.

	return *this;
}

BoneController::~BoneController()
{

}


void BoneController::aimDownSightIfPossible(){
	armMotionQueue.updateState(ArmMotionStates::handsFollowItem);
}

void BoneController::holsterWeaponIfPossible(){
	armMotionQueue.updateState(ArmMotionStates::holsterItem);
}

void BoneController::contactStateIfPossible(){
	armMotionQueue.updateState(ArmMotionStates::kontaktStellung);
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
	//setup
    leg1.setupBones(legScaleCM);
	leg2.setupBones(legScaleCM);

	arm1.setupBones(armScaleCM);
	arm2.setupBones(armScaleCM);

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

	FVector weaponSocketFront(armScaleCM * 0.8f, 0.0f, armScaleCM); //same height as arm, forward is X
	weaponMatrixOffset.setTranslation(weaponSocketFront);
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

	legDoubleKeys_1.setAnimationA(MoveTemp(legAnimationKeys));
	legDoubleKeys_2.setAnimationA(MoveTemp(legKeysCopy));

	FVector hipTargetFromFoot = FVector(0, 0, legScaleCM);
	legDoubleKeys_1.setAnimationBAdjustPermanentTarget(hipTargetFromFoot);
	legDoubleKeys_2.setAnimationBAdjustPermanentTarget(hipTargetFromFoot);


	//set aniatmion running true or false, 
	legDoubleKeys_1.setRunning(isRunning); //member var from this
	legDoubleKeys_2.setRunning(isRunning);




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
	FVector targetAim(armScaleCM * 0.8f, 0, armScaleCM);
	aimingState.setLocationAndRotation(targetAim, rotationForTarget);
	armMotionQueue.addTarget(ArmMotionStates::handsFollowItem, aimingState);


	//other states will follow of here for a holstered weapong for example
	MotionAction contactState;
	FRotator rotationForTarget2;
	rotationForTarget2.Pitch = 45; //45 degree to front
	FVector targetContactStateLocation(armScaleCM * 0.25f, 0, armScaleCM);
	contactState.setLocationAndRotation(targetContactStateLocation, rotationForTarget2);
	armMotionQueue.addTarget(ArmMotionStates::kontaktStellung, contactState);

	

	MotionAction holsterState;
	FRotator rotationForTarget3;
	rotationForTarget2.Pitch = -90; //45 degree to front
	FVector targetHolsterStateLocation(0, armScaleCM * 0.1f, 0);
	contactState.setLocationAndRotation(targetHolsterStateLocation, rotationForTarget3);
	armMotionQueue.addTarget(ArmMotionStates::holsterItem, holsterState);

}


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

void BoneController::attachTorso(AActor *torsoPointerIn){
	if(torsoPointerIn != nullptr){
		attachedTorso = torsoPointerIn;
	}
}






void BoneController::drawBody(float DeltaTime){
	MMatrix current = currentTransform();
	MMatrix shoulder1 = currentTransform(SHOULDER_1);
	MMatrix shoulder2 = currentTransform(SHOULDER_2);
	MMatrix foot1 = currentTransform(FOOT_1);
	MMatrix foot2 = currentTransform(FOOT_2);

	FVector com = current.getTranslation();

	DeltaTime *= 1.1f;

	DebugHelper::showLineBetween(GetWorld(), com, shoulder1.getTranslation(), FColor::Red, DeltaTime);
	DebugHelper::showLineBetween(GetWorld(), com, shoulder2.getTranslation(), FColor::Red, DeltaTime);
	DebugHelper::showLineBetween(GetWorld(), com, foot1.getTranslation(), FColor::Red, DeltaTime);
	DebugHelper::showLineBetween(GetWorld(), com, foot2.getTranslation(), FColor::Red, DeltaTime);
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



void BoneController::SetLocation(FVector &pos){
    //setup
    ownLocation.setTranslation(pos);

    ownLocationFoot1.setTranslation(pos);
    ownLocationFoot2.setTranslation(pos);

	ownLocationHand1.setTranslation(pos);
    ownLocationHand2.setTranslation(pos);

}

FVector BoneController::GetLocation(){
	return currentTransform().getTranslation();
}

/// @brief is not tested!
/// @param TargetLocation 
void BoneController::LookAt(FVector TargetLocation) 
{
    // Calculate the rotation needed to look at the target location
	MMatrix transform = currentTransform();
	FVector location = transform.getTranslation();
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(location, TargetLocation);

	//FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), TargetLocation);
	float zDegree = LookAtRotation.Yaw;

	ownOrientation.yawRad(MMatrix::degToRadian(zDegree));
}






void BoneController::yawRotate(float degree){
	float rad = MMatrix::degToRadian(degree);
	ownOrientation.yawRad(rad);
}

/// @brief method to attach a carried item to the hands of the actor
/// @param carriedItem 
void BoneController::attachCarriedItem(AcarriedItem *carriedItem){
	if(carriedItem != nullptr){
		attachedCarriedItem = carriedItem;
		attachedCarriedItem->SetActorEnableCollision(false);
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





void BoneController::SetControllerState(BoneControllerStates state){
    currentMotionState = state;
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

	if(currentMotionState == BoneControllerStates::none){
        TickBuildNone(DeltaTime);
    }
    if(currentMotionState == BoneControllerStates::locomotion){
        TickLocomotion(DeltaTime);
		TickArms(DeltaTime);
	}
	if(currentMotionState == BoneControllerStates::locomotionClimbAll){
		TickLocomotionClimbAll(DeltaTime);
	}

}



/// @brief Tick Locomotion Based on State 
/// @param DeltaTime 
void BoneController::TickLocomotion(float DeltaTime){

	
	if (leg1isPlaying)
	{
		buildRawAndKeepEndInPlace(leg2, ownLocationFoot2, DeltaTime, leg2Color, FOOT_2);
		playForwardAndBackwardKinematicAnim(leg1, legDoubleKeys_1, ownLocationFoot1, DeltaTime, leg1Color, FOOT_1);

		//switch to next leg but update relative position
		if(legDoubleKeys_1.animationCycleWasComplete()){
			leg1isPlaying = false;

			//die hip wurde mit dem anderen bein bewegt /(leg 1, also muss leg2 geupdated werden)
			FVector footPos = ownLocationFoot2.getTranslation();
			DebugHelper::showLineBetween(world, footPos, currentTransform(FOOT_2).getTranslation(), FColor::Orange, 2.0f);


			transformFromWorldToLocalCoordinates(footPos, FOOT_2);
			legDoubleKeys_2.overrideCurrentStartingFrame(footPos); //update starting pos of the foot because the actor moved

			

			
		}
	}else{
		
		buildRawAndKeepEndInPlace(leg1, ownLocationFoot1, DeltaTime, leg1Color, FOOT_1);
		playForwardAndBackwardKinematicAnim(leg2, legDoubleKeys_2, ownLocationFoot2, DeltaTime, leg2Color, FOOT_2);

		if(legDoubleKeys_2.animationCycleWasComplete()){//left leg done
			leg1isPlaying = true;

			//die hip wurde mit dem anderen bein bewegt /(leg 2, also muss leg1 geupdated werden)
			FVector footPos = ownLocationFoot1.getTranslation();
			DebugHelper::showLineBetween(world, footPos, currentTransform(FOOT_1).getTranslation(), FColor::Orange, 2.0f);
			
            transformFromWorldToLocalCoordinates(footPos, FOOT_1);
			legDoubleKeys_1.overrideCurrentStartingFrame(footPos); //update starting pos of the foot because the actor moved

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


/// @brief will tick the arm movement and exclude arms
/// if they are climbing
/// call this method for any arm related tick
/// @param DeltaTime 
void BoneController::TickArms(float DeltaTime){

	

	/**
	 * arm holing item at least one handed if attached
	 */
	if(attachedCarriedItem != nullptr){

		bool NEWSECTION = true;
		if (!NEWSECTION){
			// move weapon by this actor for now
			MMatrix weaponWorldMatrix = currentTransform(weaponMatrixOffset);
			FVector locationWeapon = weaponWorldMatrix.getTranslation();
			attachedCarriedItem->SetActorLocation(locationWeapon); //update location
			FRotator rotator = ownOrientation.extractRotator();
			attachedCarriedItem->SetActorRotation(rotator);


			//attach hand
			FVector targetWorld = attachedCarriedItem->rightHandLocation(); // arm 2 (?)
			moveBoneAndSnapEndEffectorToTarget(
				SHOULDER_2,
				DeltaTime,
				targetWorld,
				FVector(0, 0, -1)
			);
		}



		
		//will be removed probably!:

		//if (!NEWSECTION && currentMotionState != BoneControllerStates::locomotionClimbAll)
		if (currentMotionState != BoneControllerStates::locomotionClimbAll)
		{
			FVector targetWorld1 = attachedCarriedItem->leftHandLocation();
			moveBoneAndSnapEndEffectorToTarget(
				SHOULDER_1,
				DeltaTime,
				targetWorld1,
				FVector(0, 0, -1)
			);
		}

		/**
		 * 
		 * 
		 * ----- NEW SECTION FOR ARMS! Ticking the arm motion queue-----
		 * 
		 * 
		 */
		if(NEWSECTION && currentMotionState != BoneControllerStates::locomotionClimbAll || 
		(currentMotionState == BoneControllerStates::locomotionClimbAll &&
		 armMotionQueue.isTransitioning())
		){
			MMatrix transform = currentTransform();
			MMatrix transformLeftArm = currentTransform(SHOULDER_1);
			MMatrix transformRightArm = currentTransform(SHOULDER_2);
			MMatrix *endEffectorLeft = findEndEffector(SHOULDER_1);
			MMatrix *endEffectorRight = findEndEffector(SHOULDER_2);

			TwoBone *leftArm = findBone(SHOULDER_1);
			TwoBone *rightArm = findBone(SHOULDER_2);
			if(
				rightArm != nullptr && leftArm != nullptr &&
				endEffectorRight != nullptr && endEffectorLeft != nullptr
			){
				armMotionQueue.Tick(
					transform, 
					transformLeftArm,
					transformRightArm,
					*endEffectorRight,
					*endEffectorLeft,
					*leftArm, 
					*rightArm, 
					attachedCarriedItem, 
					DeltaTime
				);

				
			}
		}
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

		FVector weight(1, 0, 0);
		FVector target = endEffector->getTranslation();
		transformFromWorldToLocalCoordinates(target, limbIndex);

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
	TwoBone &bone, 
	KeyFrameAnimation &frames, 
	float DeltaTime,
	int limbIndex
){

	MMatrix initalTransform = currentTransform(limbIndex);
	FColor color = limbColor(limbIndex);

	//testing
	FVector weight(-1, 1, 0); //testing needed for arm rotation
	FVector nextPos = frames.interpolate(DeltaTime); //new function
	
	//dummy adjust, not foot or end, nothing for now. Could be hand.
	MMatrix dummy;

	MMatrix *endEffector = findEndEffector(limbIndex);
	MMatrix &ref = dummy; //is dummy if none found
	if(endEffector != nullptr){
		ref = *endEffector;
	}

	//todo hier: end effector einfügen!

	bone.rotateEndToTargetAndBuild(
		GetWorld(),
		nextPos,
		weight,
		initalTransform, //offset and rotation
		ref, //end effector to update
		color,
		DeltaTime * 2.0f
	);
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
	von hier das bein so rendern das bein am originalen punkt bleibt
	das foot transform muss also zu einem target umgewandelt werden
	und dann dort hin bewegt. Egal ob was neu berechnet wird oder nicht
	*/

	FVector weight(1, 0, 0); // gewicht zieht nach vorne
	
	FVector pos = legEndTransform.getTranslation();
    MMatrix currenttransform = currentTransform(leg);
	currenttransform.transformFromWorldToLocalCoordinates(pos); //relativ zur hüfte neu berechnen, auchwenn hüfte sich bewegt!

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
    MMatrix current = currentTransform(leg); //based on leg transform
	
	float velocityT = addVelocityBasedOnState(); 
    FVector lookDir = currentTransform().lookDirXForward();
	lookDir.Z = 0.0f; //xy pane only of interest

	bool switchToClimbLocomotion = false;
	frames.projectNextFrameIfNeeded(
		GetWorld(), 
		current,
		velocityT,
		lookDir,
		switchToClimbLocomotion, //boolean by reference, switch needed or not
		legScaleCM, //max height 
		currentMotionState
	);
	
	//override motion state if needed
	if(switchToClimbLocomotion && currentMotionState == BoneControllerStates::locomotion){
		currentMotionState = BoneControllerStates::locomotionClimbAll; //switch to climb motion

		//put item to holster
		armMotionQueue.updateState(ArmMotionStates::holsterItem);
	}

	//running velocity
	FVector thisAdd = lookDir * velocityT * DeltaTime;
	ownLocation += thisAdd; 




	/**
	 * HIP PROJECTION - GRAVITY
	 */
	FVector endEffector = footMatrix.getTranslation();
	FVector hipoffsetAdd = frames.getProjectionHipOffsetTimed(DeltaTime, endEffector);
	ownLocation += hipoffsetAdd; 




	//do movement
	//nochmal neu rechnen nach offset
    MMatrix translationActor = currentTransform(leg);	
	
	FVector weight(1, 0, 0);
	

	//interpolate World is used to play any animation - even if no target is setup!
	FVector EndEffectorWorld = footMatrix.getTranslation();
	FVector nextPos = frames.interpolateWorld(
		DeltaTime,
		EndEffectorWorld,
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
	
	FVector hipoffsetAdd = frames.getProjectionHipOffsetTimed(DeltaTime, footMatrix.getTranslation());
	footMatrix += hipoffsetAdd;
	

	
	//new live update local position at all times
	FVector StartEffectorWorld = currentTransform(leg).getTranslation(); //hip start
	MMatrix inverse = currentFootTransform(footMatrix).jordanInverse(); //foot local system
	FVector startEffectorLocal = inverse * StartEffectorWorld; //hip relative to current foot / end effector pos

	//interpolated with live location update
	FVector xt = frames.interpolate(DeltaTime, startEffectorLocal); 
	
	MMatrix current_RotatedFoot = currentFootTransform(footMatrix);

    //hip matrix aber da wo der fuss attached ist am oberschenkel
    MMatrix ownLocationRelativeToHip = currentTransform(leg);
    
    //MMatrix current_translationActorFoot = currentFootTransform(footMatrix);
	bone.rotateStartToTargetAndBuild(
		GetWorld(),
		xt,
		weight,
		current_RotatedFoot, // foot start
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
 * function call. Prevents bugs. All other animations will fix them selve because
 * the world frame is passed and updates the animation dynamically later
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

					FString text = FString::Printf(TEXT("blocked backward adjust by  %d"), limbIndex);
					DebugHelper::logMessage(text);
				}
			}
		}
	}
	
}


