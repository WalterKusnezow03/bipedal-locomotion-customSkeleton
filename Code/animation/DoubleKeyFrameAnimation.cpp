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
                updateInterpolatorB(aReachedTickFrame);
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






// ---- new testing gravity maker ----
FVector DoubleKeyFrameAnimation::getProjectionOffsetTimed(
    float DeltaTime, 
    FVector currentEndEffector
){
    return gravityInterpolator.interpolate(currentEndEffector, DeltaTime);
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
    FrameProjectContainer &container,
    bool &switchToArmLocomotion,
    float maxHeightSwitch,
    BoneControllerStates locomotionType
){

    bool wasProjected = framesA.projectNextFrameToGroundIfNeeded(
        container
    );

    FVector offsetMade = container.getOffsetFromOriginal();
    FVector worldHitPoint = container.getWorldHit();

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
    velocityOfActor = container.getVelocity();
    if(wasProjected){
        processProjectOffset(offsetMade);

        if(!switchToArmLocomotion)
            gravityInterpolator.updateGroundPosition(worldHitPoint);
    }

}




void DoubleKeyFrameAnimation::forceRefreshTarget(
    FrameProjectContainer &container
){
    if(isAnimationA()){
        
        framesA.forceRefreshTarget(
            container
        );

        FVector offsetMade = container.getOffsetFromOriginal();
        FVector worldHitPoint = container.getWorldHit();

        processProjectOffset(offsetMade);
        gravityInterpolator.updateGroundPosition(worldHitPoint); //NOT IMPLEMENTED YET!

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
                updateInterpolatorB(aReachedTickFrame);
            }
            
        }


    }
    else{

        interpolateB.overrideStartSpeedRelative(currentRelative);
        interpolated = interpolateB.interpolate(DeltaTime);

        //interpolated = interpolateB.interpolate(DeltaTime, currentRelative);
        
        //new: alos listen for ground reached on B!
        if(interpolateB.hasReachedTarget() && gravityInterpolator.groundReachedFlag()){
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
                updateInterpolatorB(aReachedTickFrame);
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




void DoubleKeyFrameAnimation::updateInterpolatorB(FVector reachedA){
    //relativer frame vom fuss zur hip
    FVector relativeBFrame = reachedA * -1;

    /**
     * 
     * angle must be measured here later too!
     * 
     */
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








/**
 * 
 * 
 * expiremental
 * 
 * 
 */
void DoubleKeyFrameAnimation::rotateNextFramesA(
    float signedAngleYawDegree
){
    framesA.rotateNextFrames(signedAngleYawDegree);
}


void DoubleKeyFrameAnimation::resetAnimationToStartAndResetRotation(){
    isAnimationAPlaying = true;
    cycleComplete = false;
    framesA.resetAnimationToStartAndResetRotation();
}