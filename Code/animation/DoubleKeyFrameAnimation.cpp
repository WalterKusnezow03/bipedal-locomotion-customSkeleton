// Fill out your copyright notice in the Description page of Project Settings.


#include "Math/UnrealMathUtility.h"
#include "p2/entities/customIk/MMatrix.h"
#include "p2/entities/customIk/bonePackage/BoneControllerStates.h"
#include "DoubleKeyFrameAnimation.h"

DoubleKeyFrameAnimation::DoubleKeyFrameAnimation()
{
    deltaTime = 0.0f;
    isRunningAnimation = false;
}

DoubleKeyFrameAnimation::~DoubleKeyFrameAnimation()
{
}




void DoubleKeyFrameAnimation::setAnimationA(KeyFrameAnimation &&other){
    framesA = MoveTemp(other);
}

void DoubleKeyFrameAnimation::setAnimationBAdjustPermanentTarget(FVector vector){
    bTarget = vector;
    bIsSetToAutoOverride = true;
}

bool DoubleKeyFrameAnimation::isAnimationA(){
    return isAnimationAPlaying;
}

bool DoubleKeyFrameAnimation::isAnimationB(){
    return !isAnimationAPlaying;
}





/// @brief interpolates A or B based on which animation is now wanted
/// @param DeltaTime delta time passed from Tick()
/// @return interpolated frame target
FVector DoubleKeyFrameAnimation::interpolate(float DeltaTime){
    
    deltaTime += DeltaTime;

    
    FVector interpolated;
    if(isAnimationA()){
        interpolated = framesA.interpolate(DeltaTime);

        //DebugHelper::showScreenMessage("A interpolating !", interpolated, FColor::Cyan);

        if(framesA.reachedLastFrameOfAnimation()){

            isAnimationAPlaying = false; //switch to B
            aReachedTickFrame = interpolated;

            //intepolierte position relativ zur hüfte ist einfach 
            //den vektor umdrehen
            if(bIsSetToAutoOverride){
                
                //relativer frame vom fuss zur hip
                FVector relativeBFrame = aReachedTickFrame * -1; 

                
                float timeToFrameB = 0.1f;
                if(velocityOfActor > 0.0f){
                    float distance = FVector::Dist(relativeBFrame, bTarget);
                    timeToFrameB = distance / velocityOfActor;
                }else{
                    timeToFrameB = framesA.totalLength();
                    timeToFrameB *= 0.5f; //debug
                }

                //interpolateB.setSpeed(animationSpeedHipAdjust);
                interpolateB.setTarget(relativeBFrame, bTarget, timeToFrameB);


                
            }
            
        }


    }
    else{
        //DebugHelper::showScreenMessage("B interpolating!", FColor::Black);
        interpolated = interpolateB.interpolate(DeltaTime);
        
        //NEW
        if(interpolateB.hasReachedTarget() && gravityInterpolator.groundReachedFlag()){
        //if(interpolateB.hasReachedTarget()){
            cycleComplete = true;
            projectionHipOffsetComplete = FVector(0, 0, 0);
            isAnimationAPlaying = true; //flip bool!
            deltaTime = 0.0f;
        }
    }


    return interpolated;
}





/// @brief will return if the animation cycle was complete
/// and reset the boolean if true
/// @return animation cycle was complete in the last interpolation (frame)
bool DoubleKeyFrameAnimation::animationCycleWasComplete(){
    if(cycleComplete){
        cycleComplete = false;
        return true;
    }
    return false;
}


/// @brief will return if the time needed is reached to finish an animation or interpolation
/// @param timeCheck 
/// @return 
bool DoubleKeyFrameAnimation::reachedTime(float timeCheck){
    if(isAnimationA()){
        return framesA.reachedLastFrameOfAnimation();
    }
    return timeCheck >= reachTime(); //interpolator B for now 

}

float DoubleKeyFrameAnimation::reachTime(){
    if(isAnimationA()){
        return framesA.totalLength();
    }else{
        return interpolateB.TimeToFrame();
    }
}













/// @brief will process the offset made from projection for hip adjustment 
///         -> use the getProjectionHipOffsetTimed() method to apply your hip offset over time
/// @param offsetMade 
void DoubleKeyFrameAnimation::processProjectOffset(FVector &offsetMade){
    if(offsetMade.Z > 0.0f){
        projectionHipOffsetComplete = FVector(0, 0, 0); //block positive offset
        return;
    }
    projectionHipOffsetComplete = offsetMade;
    
}




/// @brief additional hip offset needed because of projected frames, will be (0,0,0) if
/// no offset needed, or any other vector if needed.
/// @return vector offset for hip to add, is timed with animation linear.
FVector DoubleKeyFrameAnimation::getProjectionHipOffsetTimed(float DeltaTime){
    /**
     * only return if hip adjust time!
     * process of leg on floor and hip adjust
     */
    if(isAnimationB()){
        float fullTime = interpolateB.TimeToFrame();

        // x => ((dist * dt) / t) = dist
        FVector offsetReturn = ((projectionHipOffsetComplete * DeltaTime) / fullTime);

        float zCopy = offsetReturn.Z;
        DebugHelper::showScreenMessage("offset returned", zCopy);

        return offsetReturn;
    }

    //NEU: Flugphase wenn Anim A und running
    return flyingOffset();
}



// ---- new testing gravity maker ----
FVector DoubleKeyFrameAnimation::getProjectionHipOffsetTimed(
    float DeltaTime, 
    FVector currentEndEffector
){
    return gravityInterpolator.interpolate(currentEndEffector, DeltaTime);
}




void DoubleKeyFrameAnimation::forceProjectNextFrame(
    UWorld *world, 
    MMatrix &actorMatrix
){
    if(world != nullptr){
        FVector offsetMade(0,0,0);
        framesA.forceProjectToGround(world, actorMatrix, offsetMade);
        //processProjectOffset(offsetMade);
    }
}


void DoubleKeyFrameAnimation::overrideCurrentStartingFrame(FVector &currentLocationRelative){
    framesA.overrideCurrentStartingFrame(currentLocationRelative); //override target interpolator start
}


/// @brief will skip the next A frames animation once completly with an start and end point!
/// @param start starting pos
/// @param end ending pos
void DoubleKeyFrameAnimation::skipAnimationOnce(FVector start, FVector end){
    framesA.skipAnimationOnce(start, end);
}






/// @brief switch from default walking mode to running mode
/// @param b enable or disable running mode for flying offset
void DoubleKeyFrameAnimation::setRunning(bool b){
    isRunningAnimation = b;
}




/// @brief will create the current flying offset if running
/// @return flying offset 
FVector DoubleKeyFrameAnimation::flyingOffset(){
    if(isAnimationA() && isRunningAnimation){
        float totalTime = framesA.totalLength();
        return FVector(
            0,
            0,
            0 * sinusFlyingOffset(deltaTime, totalTime) //ist noch verbuggt, actor zu weit in der luft
        );
    }
    return FVector(0, 0, 0); //kein sprung ansonsten
}



float DoubleKeyFrameAnimation::sinusFlyingOffset(float time, float width){

    float slice = (PI / width); //Pi ist die zweite nullstelle
    float scaleDownFactor = 0.5f; //höhe skallieren

    float direction = time > (width * 0.5f) ? -1.0f : 1.0f; //nach der hälfte gehts runter

    return std::sin(slice * time) * direction * scaleDownFactor;
}







/**
 * 
 * 
 * --- PROJECTION SECTION ---
 * 
 * 
 */


void DoubleKeyFrameAnimation::projectNextFrameIfNeeded(
    UWorld *world, 
    MMatrix &actorMatrix
){
    bool b = false;
    float infheight = 99999.0f;
    float velocity = 0.0f;
    FVector noneDir(0, 0, 0);
    projectNextFrameIfNeeded(
        world, 
        actorMatrix, 
        velocity, 
        noneDir, 
        b, 
        infheight, 
        BoneControllerStates::none
    );
}



/// @brief will project the next frame to the floor 
/// @param world world to project in
/// @param actorMatrix actor matrix (where the actor is / animated part)
/// @param velocity velocity, can be 0.0f
/// @param lookdir look dir of velocity, distance will not be affected if velocity is 0.0 for example
/// @param switchToArmLocomotion need to switch to arm locomotion output if maxHeightSwitch exceeded
/// @param maxHeightSwitch max height before switching locomotion boolean
/// @param locomotionType if the locomotion type is climbing, none hip extra offset will be saved
/// @param worldHitPoint will save the world hitpoint relative to actors speed (next world ground target)
/// because the arm is moving differently from a leg / hip
void DoubleKeyFrameAnimation::projectNextFrameIfNeeded(
    UWorld *world,
    MMatrix actorMatrix, //is value pass on purpose
    float velocity,
    FVector &lookdir,
    bool &switchToArmLocomotion,
    float maxHeightSwitch,
    BoneControllerStates locomotionType
){

    if(world != nullptr){

        FVector removeFlyOffset = flyingOffset();
        actorMatrix -= removeFlyOffset;

        FVector offsetMade(0,0,0);
        FVector worldHitPoint(0, 0, 0);
        bool wasProjected = framesA.projectNextFrameToGroundIfNeeded(
            world,
            actorMatrix,
            offsetMade,
            velocity,
            lookdir,
            worldHitPoint
        );

        //locomotion climbing required to get to this height!
        if(offsetMade.Z > maxHeightSwitch){
            switchToArmLocomotion = true;
            offsetMade = FVector(0, 0, 0); //reset, switched to arm locomotion
        }
        if(locomotionType == BoneControllerStates::locomotionClimbAll)
        { //dont apply anything if climbing keys
            offsetMade = FVector(0, 0, 0);
        }
        
        //hier zusätzlich neue velocity speichern
        velocityOfActor = velocity;
        if(wasProjected){
            processProjectOffset(offsetMade);
            gravityInterpolator.updateGroundPosition(worldHitPoint);
        }
    }
}









/// @brief interpolates A or B based on which animation is now wanted
/// @param DeltaTime delta time passed from Tick()
/// @return interpolated frame target
FVector DoubleKeyFrameAnimation::interpolate(float DeltaTime, FVector currentRelative){
    
    deltaTime += DeltaTime;

    
    FVector interpolated;
    if(isAnimationA()){
        
        interpolated = framesA.interpolate(DeltaTime, currentRelative);

        //DebugHelper::showScreenMessage("A interpolating !", interpolated, FColor::Cyan);

        if(framesA.reachedLastFrameOfAnimation()){

            isAnimationAPlaying = false; //switch to B
            aReachedTickFrame = interpolated;

            //intepolierte position relativ zur hüfte ist einfach 
            //den vektor umdrehen
            if(bIsSetToAutoOverride){
                
                //relativer frame vom fuss zur hip
                FVector relativeBFrame = aReachedTickFrame * -1;

                
                //interpolateB.setSpeed(animationSpeedHipAdjust);

                float timeToFrameB = 0.1f;
                if(velocityOfActor > 0.0f){
                    float distance = FVector::Dist(relativeBFrame, bTarget);
                    timeToFrameB = distance / velocityOfActor;
                }else{
                    timeToFrameB = framesA.totalLength();
                    timeToFrameB *= 0.5f; //debug
                }
                interpolateB.setTarget(relativeBFrame, bTarget, timeToFrameB);


                
            }
            
        }


    }
    else{

        interpolateB.overrideStartSpeedRelative(currentRelative);
        interpolated = interpolateB.interpolate(DeltaTime);

        //interpolated = interpolateB.interpolate(DeltaTime, currentRelative);
        
        //new: alos listen for ground reached on B!
        if(interpolateB.hasReachedTarget() && gravityInterpolator.groundReachedFlag()){

        
        //if(interpolateB.hasReachedTarget()){
            cycleComplete = true;
            projectionHipOffsetComplete = FVector(0, 0, 0);
            isAnimationAPlaying = true; //flip bool!
            deltaTime = 0.0f;
        }
    }


    return interpolated;
}




/**
 * 
 * 
 * 
 * ---- WORLD SECTION ----
 * 
 * 
 */

/// @brief will override the world and local animation
/// @param actor actor world pos to make frames relative, both world and local keyframes are made
/// @param start world pos, will be relative to actor
/// @param end world pos, will be relative to actor
void DoubleKeyFrameAnimation::skipAnimationOnceWorld(MMatrix &actor, FVector start, FVector end){
    framesA.skipAnimationOnceWorld(actor, start, end);
}


/// @brief will interpolate to target World if possible and switch to local / default animation
/// if an issue occured
/// @param DeltaTime 
/// @param currentPosWorld 
/// @param actor 
/// @return 
FVector DoubleKeyFrameAnimation::interpolateWorld(
    float DeltaTime, 
    FVector currentPosWorld, 
    MMatrix &actor //limb start matrix
){
    deltaTime += DeltaTime;

    
    FVector interpolated;
    if(isAnimationA()){
        
        
        interpolated = framesA.interpolateWorld(
            DeltaTime,
            currentPosWorld,
            actor
        );

        //DebugHelper::showScreenMessage("A interpolating !", interpolated, FColor::Cyan);

        if(framesA.reachedLastFrameOfAnimation()){

            isAnimationAPlaying = false; //switch to B
            aReachedTickFrame = interpolated;

            //intepolierte position relativ zur hüfte ist einfach 
            //den vektor umdrehen
            if(bIsSetToAutoOverride){
                
                //relativer frame vom fuss zur hip
                FVector relativeBFrame = aReachedTickFrame * -1;

                
                //interpolateB.setSpeed(animationSpeedHipAdjust);

                float timeToFrameB = 0.1f;
                if(velocityOfActor > 0.0f){
                    float distance = FVector::Dist(relativeBFrame, bTarget);
                    timeToFrameB = distance / velocityOfActor;
                }else{
                    //very random.
                    timeToFrameB = framesA.totalLength();
                    timeToFrameB *= 0.5f; //debug
                }
                interpolateB.setTarget(relativeBFrame, bTarget, timeToFrameB);


                
            }
            
        }


    }
    else{

        /*
        ACHTUNG: HIER NOCH NUR LOKAL!
        ---> kann auch so bleiben weil die hüfte eher immer eine relative position zum
             end effector hat als eine globale. 
             etwas anderes macht in diesem anwendungsfall keinen sinn.
        
        */
        MMatrix inverse = actor.jordanInverse();
        FVector currentRelative = inverse * currentPosWorld;

        interpolateB.overrideStartSpeedRelative(currentRelative);
        interpolated = interpolateB.interpolate(DeltaTime);

        //interpolated = interpolateB.interpolate(DeltaTime, currentRelative);
        if(interpolateB.hasReachedTarget()){
            cycleComplete = true;
            projectionHipOffsetComplete = FVector(0, 0, 0); //reset offset vector
            isAnimationAPlaying = true; //flip bool!
            deltaTime = 0.0f;
        }
    }
    return interpolated;


}