// Fill out your copyright notice in the Description page of Project Settings.


#include "MotionQueue.h"

MotionQueue::MotionQueue()
{
    updateState(ArmMotionStates::handsFollowItem); //default at start
    transitioning = false;
    hipInterpolator.setNewTimeToFrame(0.5f); //debug wise
}

MotionQueue::~MotionQueue()
{
}

bool MotionQueue::isTransitioning(){
    return transitioning;
}

/// @brief adds an action for the wanted state and overrides the old one
/// @param state state from enum
/// @param action action with position and rotation for the item
void MotionQueue::addTarget(ArmMotionStates state, MotionAction action){
    statesMap[state] = action;
}

/// @brief updates the motion state if possible (animation and state changed or not)
/// @param state 
void MotionQueue::updateStateIfPossible(ArmMotionStates state){
    if(state != currentState && !transitioning){
        updateState(state);
    }
}


/// @brief force overrides the current motion state
/// @param state 
void MotionQueue::updateState(ArmMotionStates state){

    transitioning = true; //update transit to true so it gets ticked later

    MotionAction current = statesMap[currentState];
    MotionAction next = statesMap[state];
    currentState = state;

    float timeToFrameDebug = 0.5f;

    interpolator.setTarget(
        current.copyPosition(),
        next.copyPosition(),
        current.copyRotation(),
        next.copyRotation(),
        timeToFrameDebug
    );
}

/// @brief will build both arms, move the carried item based on the state (or not) relative
/// to the transform actor matrix, and move the arms accordingly
/// @param transform transform of the whole actor, hip
/// @param transformLeftArm transform of the left arm starting joint
/// @param transformRightArm transform of the right arm starting joint
/// @param endEffectorRight end effector of the right arm (will be modified by the bone)
/// @param endEffectorLeft end effector of the left arm (will be modified by the bone)
/// @param leftArm left arm bone to move and build
/// @param rightArm right arm bone to move and build
/// @param item item to carry
/// @param DeltaTime DeltaTime
void MotionQueue::Tick(
    MMatrix &transform, 
    MMatrix &transformLeftArm,
    MMatrix &transformRightArm,
    MMatrix &endEffectorRight,
    MMatrix &endEffectorLeft,
    TwoBone &leftArm, 
    TwoBone &rightArm, 
    AcarriedItem *item, 
    UWorld *world,
    float DeltaTime
){

    if(item != nullptr){

        if (transitioning)
        {

            FVector currentLocationWorldWillBeLocal = item->GetActorLocation();
            transform.transformFromWorldToLocalCoordinates(currentLocationWorldWillBeLocal);

            //interpolate override start local
            //dann interpolieren
            interpolator.overrideStartSpeedRelative(currentLocationWorldWillBeLocal);

            //hier muss interpolator auch noch angetickt werden
            FRotator rotation;
            FVector posLocal = interpolator.interpolate(DeltaTime, rotation); //interpoliert immer in local space
            FVector posWorld = transform * posLocal;

            //actor and wanted rotation combined for the carried item
            MMatrix rotatorMatrix = MMatrix::createRotatorFrom(rotation);
            MMatrix transformCopy = transform;
            transformCopy.setTranslation(0, 0, 0);
            transformCopy = transformCopy * rotatorMatrix; //M = B * A <-- lese richtung --

            FRotator finalRotation = transformCopy.extractRotator();


            item->SetActorRotation(finalRotation);
            item->SetActorLocation(posWorld);
            

            //wenn fertig interpoliert, abbruch. Klar.
            if(interpolator.hasReachedTarget()){
                transitioning = false;
            }
        }
        else
        {

            //default follow
            MotionAction *currentStatePointer = &statesMap[currentState];
            if(currentStatePointer != nullptr){

                
                //setting up data for the weapon transform
                MMatrix rotationRein = transform;
                rotationRein.setTranslation(0, 0, 0); //make pure rotation matrix
                MMatrix rotation = currentStatePointer->copyRotationAsMMatrix();
                rotationRein *= rotation; //erst skellet dann target rotation

                FRotator finalRotation = rotationRein.extractRotator();
                item->SetActorRotation(finalRotation);

                FVector location = currentStatePointer->copyPosition();
                FVector posWorld = transform * location;
                item->SetActorLocation(posWorld);
                
            }else{
                DebugHelper::showScreenMessage("ISSUE!!!", FColor::Red);
            }
        }

        




        FVector rightHandtarget = item->rightHandLocation();
        FVector leftHandtarget = item->leftHandLocation();

        //DebugHelper::showLineBetween(item->GetWorld(), rightHandtarget, rightHandtarget + FVector(0, 0, 50));
        //DebugHelper::showLineBetween(item->GetWorld(), leftHandtarget, leftHandtarget + FVector(0, 0, 50));

        FVector weight(0, 0, -1);

        //dont move arms if state is none
        if(handsAtItem()){
            moveBoneAndSnapEndEffectorToTarget(
                DeltaTime,
                leftHandtarget,
                weight,
                transformLeftArm, // transform limb start
                endEffectorLeft,
                leftArm,
                world
            );

            moveBoneAndSnapEndEffectorToTarget(
                DeltaTime,
                rightHandtarget,
                weight,
                transformRightArm, //transform limb start
                endEffectorRight,
                rightArm,
                world
            );
        }
    }else{
        //default build bones if item is null!

        moveAndBuildBone(
            DeltaTime,
            transformLeftArm, // shoulder start
            endEffectorLeft,
            leftArm,
            world
        );

        moveAndBuildBone(
            DeltaTime,
            transformRightArm, // shoulder start
            endEffectorRight,
            rightArm,
            world
        );

    }

    
    

}

/// @brief will return wheter the hands must be moved to the carried item
/// @return true / false move hands to item
bool MotionQueue::handsAtItem(){
    bool alreadyHolstered = (currentState == ArmMotionStates::holsterItem) && !isTransitioning();
    if(alreadyHolstered){
        return false;
    }

    return true;
}

void MotionQueue::moveBoneAndSnapEndEffectorToTarget(
    float DeltaTime,
    FVector targetWorld,
    FVector weight,
    MMatrix &translationActor,
    MMatrix &endEffector,
    TwoBone &bone,
    UWorld *world
){
    weight = weight.GetSafeNormal();

    translationActor.transformFromWorldToLocalCoordinates(targetWorld);

	bone.rotateEndToTargetAndBuild(
		world,
		targetWorld, //as local now
		weight,
		translationActor, // hip start with orient
		endEffector, //ownLocationFoot,  // foot apply positions
		FColor::Red, 
		DeltaTime * 2.0f
	);
}



void MotionQueue::moveAndBuildBone(
    float DeltaTime,
    MMatrix &translationActor, //shoulder start
    MMatrix &endEffector,
    TwoBone &bone,
    UWorld *world
){

    bone.build(
        world,
        translationActor,
        endEffector,
        FColor::Red,
        DeltaTime
    );
}









/**
 * 
 * leg type of motion states
 * 
 */




MMatrix MotionQueue::TickUpdatedHipMoveAlignMatrix(
    MMatrix &hipJointMatStartRotated,
    MMatrix &orientation,
    MMatrix &endEffector,
    TwoBone &bone1,
    float DeltaTime,
    float signedYawAngleAddedToFrames,
    UWorld *world,
    bool &reachedFlag,
    float velocityOfMovement
){
    FVector targetRelativeToEnd = bone1.startRelativeToEnd_Initial();
    FVector jointStartWorld = hipJointMatStartRotated.getTranslation();
    FVector jointTargetWorld = endEffector.getTranslation() + targetRelativeToEnd;
    //direction of hip to target
    FVector direction = jointTargetWorld - jointStartWorld;

    if(reached(jointStartWorld, jointTargetWorld)){
        if(std::abs(signedYawAngleAddedToFrames) <= 0.001f){
            reachedFlag = true;
            resetHipInterpolatorAndFlags();
            return hipJointMatStartRotated;
        }
        
    }

    reachedFlag = false;
    hipTransitioning = true;
    if(!setupHipTarget){
        FRotator orientationExtracted = orientation.extractRotator();
        FRotator orientationRotated = orientationExtracted;
        orientationRotated.Yaw += signedYawAngleAddedToFrames;

        setupHipTarget = true;

        /*
         * equation solve for s:
         * 
         * m/s = m/s
         * v = dist / s
         * s = dist / v
         *  
         */

        float dist = FVector::Dist(jointStartWorld, jointTargetWorld);
        velocityOfMovement = std::max(std::abs(velocityOfMovement), 0.01f);
        float timeToFrame = dist / velocityOfMovement;

        hipInterpolator.setTarget(
            jointStartWorld,
            jointTargetWorld,
            orientationExtracted,
            orientationRotated,
            timeToFrame//0.3f
        );

        
    }
    
    MMatrix newHipOffsetted = hipInterpolator.interpolateAndGenerateTransform(DeltaTime);

    DebugHelper::showLineBetween(
        world,
        newHipOffsetted.getTranslation(),
        newHipOffsetted.getTranslation() + FVector(0,0,50),
        FColor::Purple,
        1.0f
    );

    if(hipInterpolator.hasReachedTarget()){
        reachedFlag = true;
        resetHipInterpolatorAndFlags();
    }
    return newHipOffsetted;



}











/// @brief resets all target flags and sets the reached flag to true
void MotionQueue::resetHipInterpolatorAndFlags(){
    setupHipTarget = false;
    hipTransitioning = false; //update transit flag
    hipInterpolator.setNewTimeToFrame(0.5f);
}




bool MotionQueue::hipInTransit(){
    return hipTransitioning;
}

bool MotionQueue::reached(FVector &a, FVector &b){
    return FVector::Dist(a, b) <= 1.0f;
}

bool MotionQueue::isParalel(MMatrix &orientation, FVector directionOfEndEffector){
    FVector forward = orientation.lookDirXForward();
    forward = forward.GetSafeNormal();
    directionOfEndEffector = directionOfEndEffector.GetSafeNormal();
    directionOfEndEffector.Z = 0.0f;
    forward.Z = 0.0f;

    return std::abs(FVector::DotProduct(forward, directionOfEndEffector)) >= 0.99f;
}


float MotionQueue::signedYawAngle(MMatrix &actorWorld, FVector actorComToEndEffector){
    FVector lookDir = actorWorld.lookDirXForward().GetSafeNormal();
    actorComToEndEffector = actorComToEndEffector.GetSafeNormal();

    lookDir.Z = 0.0f;
    actorComToEndEffector.Z = 0.0f;

    float dotproduct = FVector::DotProduct(lookDir, actorComToEndEffector);
    float sign = lookDir.X * actorComToEndEffector.Y - lookDir.Y * actorComToEndEffector.X < 0.0f ? -1.0f : 1.0f;


    float angle = MMatrix::radToDegree(std::acosf(dotproduct)) * sign;
    return angle;
}
