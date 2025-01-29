// Fill out your copyright notice in the Description page of Project Settings.


#include "MotionQueue.h"

MotionQueue::MotionQueue()
{
    updateState(ArmMotionStates::handsFollowItem); //default at start
    transitioning = false;
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
    float DeltaTime
){

    if(item != nullptr){
        if(transitioning){

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
            transformCopy *= rotatorMatrix;

            FRotator finalRotation = transformCopy.extractRotator();

            item->SetActorLocation(posWorld);
            item->SetActorRotation(finalRotation);

            //wenn fertig interpoliert, abbruch. Klar.
            if(interpolator.hasReachedTarget()){
                transitioning = false;
                /*if(currentState == ArmMotionStates::holsterItem){
                    currentState = ArmMotionStates::none;
                }*/
            }
        }else{
            
            //default follow
            MotionAction *currentStatePointer = &statesMap[currentState];
            if(currentStatePointer != nullptr){

                //MUST BE CLEANED AND REFACTURED!
                //setting up data for the weapon transform                
                FRotator rotation = currentStatePointer->copyRotation();

                MMatrix rotatorMatrix = MMatrix::createRotatorFrom(rotation);
                rotatorMatrix *= transform;

                FRotator finalRotation = rotatorMatrix.extractRotator();

                FVector location = currentStatePointer->copyPosition();
                //FVector posWorld = rotatorMatrix * location; //test statt transform
                FVector posWorld = transform * location; //test statt transform


                item->SetActorLocation(posWorld);
                item->SetActorRotation(finalRotation);

                DebugHelper::showScreenMessage("default update motion queue", FColor::Green);
            }else{
                DebugHelper::showScreenMessage("ISSUE!!!", FColor::Red);
            }
        }






        FVector rightHandtarget = item->rightHandLocation();
        FVector leftHandtarget = item->leftHandLocation();
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
                item->GetWorld()
            );

            moveBoneAndSnapEndEffectorToTarget(
                DeltaTime,
                rightHandtarget,
                weight,
                transformRightArm, //transform limb start
                endEffectorRight,
                rightArm,
                item->GetWorld()
            );
        }else{
            moveAndBuildBone(
                DeltaTime,
                transformLeftArm, // shoulder start
                endEffectorLeft,
                leftArm,
                item->GetWorld()
            );

            moveAndBuildBone(
                DeltaTime,
                transformRightArm, // shoulder start
                endEffectorRight,
                rightArm,
                item->GetWorld()
            );
        }
    }
    

}

/// @brief will return wheter the hands must be moved to the carried item
/// @return true / false move hands to item
bool MotionQueue::handsAtItem(){
    bool alreadyHolstered = (currentState == ArmMotionStates::holsterItem) && !isTransitioning();
    bool isNoneState = (currentState == ArmMotionStates::none);
    if (alreadyHolstered || isNoneState)
    {
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