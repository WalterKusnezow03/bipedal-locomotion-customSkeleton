// Fill out your copyright notice in the Description page of Project Settings.


#include "p2/entities/customIk/animation/TargetInterpolator.h"
#include "p2/entities/customIk/animation/KeyFrameAnimation.h"
#include "p2/entities/customIk/MMatrix.h"

KeyFrameAnimation::KeyFrameAnimation()
{
    frameIndex = 0;
    nextFrameIndex = 1;
    totalLengthSave = 0.0f;
    loop = true;

    //interpolator.setSpeed(50.0f); //100
}

KeyFrameAnimation::KeyFrameAnimation(bool loopIn){
    frameIndex = 0;
    nextFrameIndex = 1;
    totalLengthSave = 0.0f;
    loop = loopIn;

    //interpolator.setSpeed(50.0f); //debug
}


KeyFrameAnimation::~KeyFrameAnimation()
{
}





/// @brief add a frame to the animation at a position and distance to previous frame
/// @param position position for frame
/// @param timeFromLastFrame time to keep from last frame in seconds
void KeyFrameAnimation::addFrame(FVector position, float timeFromLastFrame){
    addFrame(position, timeFromLastFrame, false);
}

void KeyFrameAnimation::addFrame(FVector position, float timeFromLastFrame, bool mustBeGrounded){
    totalLengthSave += timeFromLastFrame;
    frames.push_back(KeyFrame(position, timeFromLastFrame, mustBeGrounded));
}

void KeyFrameAnimation::addFrame(
    FVector position,
    float timeFromLastFrame,
    bool mustBeGrounded,
    float clampDistance
){
    FVector zeroVec(0, 0, 0);
    if(FVector::Dist(zeroVec, position) > clampDistance){
        position = position.GetSafeNormal() * clampDistance;
    }

    totalLengthSave += timeFromLastFrame;
    frames.push_back(KeyFrame(position, timeFromLastFrame, mustBeGrounded));
}





/// @brief total length time of the animation
/// @return total length time of the animation
float KeyFrameAnimation::totalLength(){
    return totalLengthSave;
}

/// @brief returns whether the end frame was reached, only is flagged if the animation
/// was not initialized with loop, NOT LOOP!
/// @return reached last frame flag
bool KeyFrameAnimation::reachedLastFrameOfAnimation(){
    if(reachedEndFrameFlag){
        reachedEndFrameFlag = false;
        return true;
    }
    return false;
}



/// @brief returns if enough frames are available for an animation (at least 2)
/// @return has enough or not
bool KeyFrameAnimation::hasAnyFrames(){
    return frames.size() >= 2; //must be at least 2!
}


/**
 * 
 * ---- INTERPOLATE DEFAULT ----
 * 
 */

FVector KeyFrameAnimation::interpolate(float DeltaTime){

    if(hasAnyFrames()){
    
        if (interpolator.hasTargetSetup() == false){
            updateFrameInterpolator();
        }

        FVector interpolated = interpolator.interpolate(DeltaTime);
        

        if(interpolator.hasReachedTarget()){    
            updateFrameIndex();
            updateFrameInterpolator();
        }

        return interpolated;
    }
    DebugHelper::showScreenMessage("not enough frames!");
    return FVector(0, 0, 0);
}   






/// @brief updates the frame index and end frame flag if reached
void KeyFrameAnimation::updateFrameIndex(){
    frameIndex = (frameIndex + 1) % frames.size();
    nextFrameIndex = (frameIndex + 1) % frames.size();

    bool isEnd = (nextFrameIndex == 0) || (frameIndex == frames.size() - 1);
    if (isEnd && !loop)
    {
        frameIndex = 0;
        nextFrameIndex = 1;
        //DebugHelper::showScreenMessage("reached end frame!", FColor::Orange);
        reachedEndFrameFlag = true;
    }

}

/// @brief call update AFTER increase / index update
void KeyFrameAnimation::updateFrameInterpolator(){

    KeyFrame &currentFrame = frames.at(frameIndex);
    KeyFrame &nextFrame = frames.at(nextFrameIndex);

    FVector currentFramePosition = currentFrame.readposition();

    //copy latest reached position if possible
    if(interpolator.hasTargetSetup() && frameIndex != 0){
        currentFramePosition = interpolator.readToPosition();
    }

    interpolator.setTarget(
        currentFramePosition, //currentFrame.readposition();
        nextFrame.readposition(),
        nextFrame.readVelocity()
    );

    //reset frame projected status
    frameIsProjected = false;
}


/// @brief will tell if the next key frame must be grounded or not
/// @return must be grounded
bool KeyFrameAnimation::nextFrameMustBeGrounded(){
    if(hasAnyFrames()){
        KeyFrame &nextFrame = frames.at(nextFrameIndex);
        return nextFrame.mustBeGrounded();
    }
    return false;
}

/// @brief copies the next frames position
/// @return position of next frame of animation
FVector KeyFrameAnimation::readNextFrame(){
    if(hasAnyFrames()){
        KeyFrame &nextFrame = frames.at(nextFrameIndex);
        return nextFrame.readposition();
    }
    return FVector(0, 0, 0);
}



/// @brief override the next frame / target value, animation is not overriden, individual to current next
/// frame!
/// @param framePos 
void KeyFrameAnimation::overrideNextFrame(FVector &framePos){
    interpolator.overrideTarget(framePos);
    frameIsProjected = true;
}





bool KeyFrameAnimation::nextFrameIsProjected(){
    return frameIsProjected;
}




/// @brief overrides the starting position of the current interpolation
void KeyFrameAnimation::overrideCurrentStartingFrame(FVector &somePosition){
    interpolator.overrideStart(somePosition); //override start pos
}

/// @brief skip the animation with a and b interpolated keys, after interpolation
/// the reached end frame flag is true as expected if not looping animation
/// @param start start frame
/// @param end end frame
void KeyFrameAnimation::skipAnimationOnce(FVector start, FVector end){
    interpolator.overrideStart(start);
    interpolator.overrideTarget(end);
    interpolator.resetDeltaTime();
    frameIndex = frames.size() - 2; 
    //next frame will be -1, end will be reached
    //next frame will be 0 by then 
}


/// @brief will override the world and local animation
/// @param actor actor world pos to make frames relative, both world and local keyframes are made
/// @param start world pos, will be relative to actor
/// @param end world pos, will be relative to actor
void KeyFrameAnimation::skipAnimationOnceWorld(MMatrix &actor, FVector start, FVector end){
    //world
    interpolator.overrideStartWorld(start);
    interpolator.overrideTargetWorld(end);

    MMatrix inverse = actor.jordanInverse();
    start = inverse * start;
    end = inverse * end;

    //relative
    skipAnimationOnce(start, end); //relative override
}




/**
 * 
 * ---- PROJECTION SECTION ----
 * 
 */

bool KeyFrameAnimation::projectNextFrameToGroundIfNeeded(UWorld *world, MMatrix &actorMatrix, FVector &offsetMade){
    if(world == nullptr){
        return false;
    }
    if(!nextFrameIsProjected() && nextFrameMustBeGrounded()){

        forceProjectToGround(world, actorMatrix, offsetMade);

        return true;
    }
    return false;
}



void KeyFrameAnimation::forceProjectToGround(UWorld *world, MMatrix &actorMatrix, FVector &offsetMade){
    if(world == nullptr){
        return;
    }
    FVector frameToProject = readNextFrame();
    FVector worldHit;
    projectToGround(world, actorMatrix, frameToProject, offsetMade, worldHit);
    overrideNextFrame(frameToProject);
    interpolator.overrideTargetWorld(worldHit);
}


/// @brief projects a frame to the ground and writes the porjection into the frame
/// @param frameToProject frame to project to floor
/// @param offsetMade offset made from frame to project to hitpoint direction
void KeyFrameAnimation::projectToGround(
    UWorld *world, MMatrix &actorTransform, FVector &frameToProject, FVector &offsetMade
){
    FVector worldOutput;
    projectToGround(
        world,
        actorTransform,
        frameToProject,
        offsetMade,
        worldOutput
    );
    interpolator.overrideTargetWorld(worldOutput);
}

/// @brief projects a frame to the ground and writes the porjection into the frame
/// @param frameToProject frame to project to floor
/// @param offsetMade offset made from frame to project to hitpoint direction
void KeyFrameAnimation::projectToGround(
    UWorld *world, 
    MMatrix &actorTransform, 
    FVector &frameToProject, 
    FVector &offsetMade,
    FVector &hitpointOutput //world hit
){

    FVector lookNone(1, 0, 0);
    projectToGround(
        world,
        actorTransform,
        frameToProject,
        hitpointOutput,
        offsetMade,
        0.0f,                   // ignored time to frame extra
        0.0f,                   // ignored running velocity extra
        lookNone                // ignored look dir of velocity
    );
}

/// @brief performs a raycast from a start, into a direction, with a max length in this method
/// @param Start start position
/// @param dir direction of interest
/// @param outputHit output hit if any hit happened
/// @return bool anything was hit
bool KeyFrameAnimation::performRaycast(UWorld *world, FVector &Start, FVector &dir, FVector &outputHit) {

    if(world == nullptr){
        return false;
    }

    float scaleOfVector = raycastScaleVector; //some random value for now
	FVector End = Start + dir * scaleOfVector; // gx = A + r (B - A)

	// Perform the raycast
	FHitResult HitResult;

	FCollisionQueryParams ignoreParams;
	/*
	CAUTION: TEAMS PARAM NOT IMPLEMETED YET!

	//params to avoid hitting any other entities
	if(EntityManager *e = worldLevel::entityManager()){
		//ignoreParams = e->getIgnoredRaycastParams(); //example for getting all
		ignoreParams = e->getIgnoredRaycastParams(getTeam());
	}*/
	bool bHit = world->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);
	



	//anything was hit
	if (bHit){
		outputHit = HitResult.ImpactPoint; //write impactpoint to output
		return true;
	}

	return false;
}







/**
 * 
 * 
 * 
 * ---- future projection ----
 * 
 * 
 */

bool KeyFrameAnimation::projectNextFrameToGroundIfNeeded(
    UWorld *world, 
    MMatrix &actorMatrix, 
    FVector &offsetMade,
    float velocity,
    FVector &lookdir
){
    FVector worldHit_notneeded;
    return projectNextFrameToGroundIfNeeded(
        world,
        actorMatrix,
        offsetMade,
        velocity,
        lookdir,
        worldHit_notneeded
    );
}

bool KeyFrameAnimation::projectNextFrameToGroundIfNeeded(
    UWorld *world, 
    MMatrix &actorMatrix, 
    FVector &offsetMade,
    float velocity,
    FVector &lookdir,
    FVector &worldHitMade
){
    if(world == nullptr){
        return false;
    }
    if(!nextFrameIsProjected() && nextFrameMustBeGrounded()){

        FVector frameToProject = readNextFrame();
        float nextTimeToFrame = interpolator.TimeToFrame(); 
        FVector worldHitOutput;

        projectToGround(
            world, 
            actorMatrix, 
            frameToProject,
            worldHitOutput,
            offsetMade, 
            nextTimeToFrame,
            velocity,
            lookdir
        );
        
        overrideNextFrame(frameToProject);
        interpolator.overrideTargetWorld(worldHitOutput);

        worldHitMade = worldHitOutput; //copy
        return true;
    }
    return false;
}

/// @brief projects a frame to the ground and writes the porjection into the frame
/// @param frameToProject frame to project to floor, will be modified!
/// @param offsetMade offset made from frame to project to hitpoint direction
void KeyFrameAnimation::projectToGround(
    UWorld *world, 
    MMatrix &actorTransform, 
    FVector &frameToProject, 
    FVector &worldHitOutput, 
    FVector &offsetMade,
    float timeToFrame,
    float velocity, //running velocity
    FVector &lookdirection //look dir of velocity
){
    if(world == nullptr){
        return;
    }
    MMatrix transform = actorTransform;
    FVector frameInWorld = transform * frameToProject;

    //hier einfach drauf rechnen? (velocity und lookdir)
    //muss in abhängigkeit mit time to frame sein.

    //x(t) = x0 + v0t + 1/2 at^2
    //simplified to
    //x(t) = x0 + v0 t
    //xthis(t) = frameToProject + lookdir * timeToFrame * velocity
    //time to frame sollte runter skallieren oder? / sollte so stimmen...
    FVector lookdir = lookdirection.GetSafeNormal();
    FVector offsetFuture = lookdir * (timeToFrame * velocity);
    frameInWorld += offsetFuture;

    //EXTRA OFFSET NEEDED HERE, sont terrain nicht berührt, könnte durchlaufen!
	frameInWorld += FVector(0, 0, raycastVerticalStartOffsetAdd);


	//project frame to floor
	FVector downVec(0, 0, -1);
	FVector hitpoint;

	bool wasHit = performRaycast(world, frameInWorld, downVec, hitpoint);
	if(wasHit){
        
        if(DEBUGDRAW_RAYCAST){
            float displayTime = 1.0f;
            DebugHelper::showLineBetween(world, hitpoint, hitpoint + FVector(0, 0, 200), FColor::Yellow, displayTime);
        }


        //world copy
        worldHitOutput = hitpoint;

        //hier den offset wieder abziehen sodass die lokale posiion stimmt
        //zukunft wieder weg rechnen / abziehen
        hitpoint -= offsetFuture;


        //inverse ins locale system
        MMatrix inverse = actorTransform.jordanInverse();
        hitpoint = inverse * hitpoint;
        //actorTransform.transformFromWorldToLocalCoordinates(hitpoint);
		
        
        offsetMade = hitpoint - frameToProject;
		frameToProject = hitpoint;

        DebugHelper::showScreenMessage(
            "RAYCAST OFFSET",
            offsetMade,
            FColor::Red
        );
    }
}






/**
 * 
 * ---- INTERPOLATE WITH LOCAL UPDATE ----
 * 
 */

/// @brief interpolate with considering current pos 
/// @param DeltaTime delta time since last frame
/// @param currentPos current local position
/// @return 
FVector KeyFrameAnimation::interpolate(float DeltaTime, FVector currentPos){

    if(hasAnyFrames()){
    
        if (interpolator.hasTargetSetup() == false){
            updateFrameInterpolator();
        }


        //interpolator.overrideStart(currentPos); //debug disabled

        interpolator.overrideStartSpeedRelative(currentPos);
        FVector interpolated = interpolator.interpolate(DeltaTime);

        if(interpolator.hasReachedTarget()){
            DebugHelper::logMessage("debugmotion interpolator update");
            updateFrameIndex();
            updateFrameInterpolator();
        }

        return interpolated;
    }
    DebugHelper::showScreenMessage("not enough frames!");
    return FVector(0, 0, 0);
}   







/**
 * 
 * 
 * ---- INTERPOLATE WITH WORLD UPDATE ----
 * 
 * 
 */


/// @brief will interpolate to target World if possible and switch to local / default animation
/// if an issue occured
/// @param DeltaTime delta time since last frame
/// @param currentPosWorld current position in world
/// @param actor actor matrix in world (with rotation) to update the world frames to local ones
/// @return interpolation based on actor matrix and current position (of end effector) in world
FVector KeyFrameAnimation::interpolateWorld(
    float DeltaTime, 
    FVector currentPosWorld, 
    MMatrix &actor //limb start matrix
){

    if(hasAnyFrames()){
    
        if (interpolator.hasTargetSetup() == false){
            updateFrameInterpolator();
        }


        interpolator.overrideStartWorldSpeedRelative(currentPosWorld, actor); //will transform to local internally
        FVector interpolated = interpolator.interpolate(DeltaTime);

        if(interpolator.hasReachedTarget()){
            DebugHelper::logMessage("debugmotion interpolator update");
            updateFrameIndex();
            updateFrameInterpolator();
        }

        return interpolated;
    }
    DebugHelper::showScreenMessage("not enough frames!");
    return FVector(0, 0, 0);


}
