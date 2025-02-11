// Fill out your copyright notice in the Description page of Project Settings.


#include "p2/entities/customIk/animation/TargetInterpolator.h"

TargetInterpolator::TargetInterpolator()
{
    reached = true;
    targetSetup = false;
    timeToFrame = 0.0f;
    prev = FVector(0, 0, 0);
}

TargetInterpolator::~TargetInterpolator()
{
}


bool TargetInterpolator::hasTargetSetup(){
    return targetSetup;
}

void TargetInterpolator::setTarget(FVector fromIn, FVector totarget, float timeToFrameIn){
    from = fromIn;
    target = totarget;
    timeToFrame = timeToFrameIn;
    deltaTime = 0.0f;
    reached = false;
    targetSetup = true;
}

void TargetInterpolator::setTarget(
	FVector fromIn, 
	FVector toTarget, 
	FRotator fromRotationIn, 
	FRotator toRotationIn, 
	float timeToFrameIn
){
    setTarget(fromIn, toTarget, timeToFrameIn);
    fromRotation = fromRotationIn;
    toRotation = toRotationIn;
}

/// @brief override target of a RUNNING ANIMATION!
/// @param totarget 
void TargetInterpolator::overrideTarget(FVector totarget){

    //DebugHelper::showScreenMessage("override target, connect:", FColor::Red);
    target = totarget;
}

void TargetInterpolator::overrideStart(FVector fromtarget){
    from = fromtarget;
}

void TargetInterpolator::overrideStart(FRotator fromRotationIn){
    fromRotation = fromRotationIn;
}


void TargetInterpolator::overrideStart(FVector fromtarget, FRotator fromRotationIn){
    overrideStart(fromtarget);
    fromRotation = fromRotationIn;
}
void TargetInterpolator::overrideTarget(FVector totarget, FRotator toRotationIn){
    overrideTarget(totarget);
    toRotation = toRotationIn;
}


void TargetInterpolator::overrideTarget(FRotator toRotationIn){
    toRotation = toRotationIn;
}

void TargetInterpolator::overrideStartSpeedRelative(FVector newStart, FRotator newRoation){
    overrideStartSpeedRelative(newStart);
    fromRotation = newRoation;
}

void TargetInterpolator::overrideStartSpeedRelative(FVector newStart){
    /**
     * was passiert hier:
     * alte distanz und time to frame ergeben velocity
     *
     * velocity: m/s
     * newDist: m
     *
     * newtime = (m) / (m/s) = m * (s/m) = s
     * 
     * metersnew / speed = m * (s/m) = s
     * 
     */

    float distanceOld = FVector::Dist(from, target);
    float speed = distanceOld / timeToFrame; //sei distanz 40m und ttf 2s, dann sinds 20ms


    // Aktualisiere Startpunkt
    from = newStart;

    // Neue Time-to-Frame berechnen
    float newDistance = FVector::Dist(from, target);
    float newTimeToFrame = newDistance / speed;

    // Neue deltaTime basierend auf dem alten Fortschritt
    timeToFrame = newTimeToFrame;

    if(timeToFrame < 0.01f){
        reached = true;
    }
}

void TargetInterpolator::overrideStartSpeedRelative(FRotator newRotation){
    float distanceOld = shorterAngleSum(fromRotation, toRotation);
    float speed = distanceOld / timeToFrame; //sei distanz 40m und ttf 2s, dann sinds 20ms

    // Aktualisiere Startrotation
    fromRotation = newRotation;

    // Neue Time-to-Frame berechnen
    float newDistance = shorterAngleSum(fromRotation, toRotation);
    float newTimeToFrame = newDistance / speed;

    // Neue deltaTime basierend auf dem alten Fortschritt
    timeToFrame = newTimeToFrame;

    if(timeToFrame <= 0.01f){
        reached = true;
    }
}








void TargetInterpolator::resetDeltaTime(){
    deltaTime = 0.0f;
    reached = false;
}

void TargetInterpolator::setNewTimeToFrame(float time){
    timeToFrame = time;
    reached = false;
}


bool TargetInterpolator::hasReachedTarget(){
    return reached;
}


FVector TargetInterpolator::interpolate(float DeltaTime){
    if(reached){
        worldtargetSetup = false;
        return target;
    }
    if(deltaTime >= timeToFrame){
        reached = true;
        deltaTime = 0.0f;
        worldtargetSetup = false;
        return target;
    }

    deltaTime += DeltaTime;
    
    
    FVector connect = target - from; // AB = B - A  
    //gx = A + r (B - A)
    //FVector interpolated = from + skalar() * connect;

    float skalarCurrent = skalar();
    
    FVector interpolated = TargetInterpolator::interpolation(from, target, skalarCurrent);
    //FVector interpolated = interpolationBezier(skalarCurrent);
    

    //wenn die richtungs vektoren anti paralell zu einander liegen
    //kann ich prüfen ob mein punkt passiert wurde
    FVector dirToB = target - interpolated;
    float dotProduct = FVector::DotProduct(dirToB.GetSafeNormal(), connect.GetSafeNormal());
    
    //is tested
    //DebugHelper::showScreenMessage("dot product: ", dotProduct);
    if (dotProduct < 0.0f)
    {
        DebugHelper::showScreenMessage("PASSED FRAME");
        // anti parellell
        reached = true;
        deltaTime = 0.0f;
        worldtargetSetup = false;
        return interpolated;
    }

   
    return interpolated;

}


FRotator TargetInterpolator::interpolateRotationOnly(float DeltaTime){
    if(deltaTime >= timeToFrame){
        reached = true;
        deltaTime = 0.0f;
        worldtargetSetup = false;
        return toRotation;
    }
    deltaTime += DeltaTime;

    float skalarCurrent = skalar();

    if(skalarCurrent >= 1.0f){
        reached = true;
        deltaTime = 0.0f;
        worldtargetSetup = false;
        return toRotation;
    }
    //DebugHelper::showScreenMessage("skalar rotation", skalarCurrent); //teilweise NAN wert, unendllich

    
    FRotator rotationOutgoing = TargetInterpolator::interpolationRotation(
        fromRotation,
        toRotation,
        skalarCurrent // current skalar
    );

    //ISSUES WITH REACHING, is not switching!!

    return rotationOutgoing;
}


FVector TargetInterpolator::interpolate(float DeltaTime, FRotator &rotationOutgoing){
    FVector outpos = interpolate(DeltaTime);
    
    float skalarCurrent = skalar();
    FRotator interpolatedRotation = TargetInterpolator::interpolationRotation(
        fromRotation, 
        toRotation, 
        skalarCurrent
    );
    rotationOutgoing = interpolatedRotation;
    return outpos;
}

MMatrix TargetInterpolator::interpolateAndGenerateTransform(float DeltaTime){
    MMatrix outMatrix;
    FRotator rotatorOut;
    FVector pos = interpolate(DeltaTime, rotatorOut);
    outMatrix.setRotation(rotatorOut);
    outMatrix.setTranslation(pos);
    return outMatrix;
}

float TargetInterpolator::skalar(){
    //hier muss ein epsilon value sein!
    //1 / 0 = unendlich
    float epsilon = 0.1f;
    if (timeToFrame <= 0 + epsilon)
    {
        return 1.0f;
    }

    float skal = deltaTime / timeToFrame; //fractionThis = this / all
    
    if(skal > 1.0f){
        skal = 1.0f;
    }
    if(skal < 0.0f){
        skal = 0.0f;
    }

    // Berechnet den Skalierungsfaktor `t`, der zwischen 0 und 1 liegt
    return skal; //t / 1 quasi.

}



/*

---- static methods -----


*/

FVector TargetInterpolator::interpolation(FVector fromIn, FVector toIn, float skalar){
    FVector connect = toIn - fromIn; // AB = B - A
    //gx = A + r (B - A)
    FVector interpolated = fromIn + skalar * connect;
    return interpolated;
}

FRotator TargetInterpolator::interpolationRotation(FRotator fromIn, FRotator toIn, float skalar){
    
    FRotator output;
    output.Roll = fromIn.Roll + skalar * rotationDirectionShorter(fromIn.Roll, toIn.Roll);
    output.Pitch = fromIn.Pitch + skalar * rotationDirectionShorter(fromIn.Pitch, toIn.Pitch);
    output.Yaw = fromIn.Yaw + skalar * rotationDirectionShorter(fromIn.Yaw, toIn.Yaw);

    return output;
}

/// @brief creates the shorter rotation direction between two angles a and b which can be signed
/// but will be clamped from -180 to 180 degrees
/// @param a 
/// @param b 
/// @return signed shorter angle
float TargetInterpolator::rotationDirectionShorter(float a, float b){
    a = std::clamp(a, -180.0f, 180.0f);
    b = std::clamp(b, -180.0f, 180.0f);

    float diffA = a - b;
    float diffB = b - a;
    if(std::abs(diffA) < std::abs(diffB)){
        return diffA;
    }
    return diffB;
}



float TargetInterpolator::shorterAngleSum(FRotator &a, FRotator &b){
    float sum = 0.0f;
    sum += std::abs(rotationDirectionShorter(a.Roll, b.Roll));
    sum += std::abs(rotationDirectionShorter(a.Pitch, b.Pitch));
    sum += std::abs(rotationDirectionShorter(a.Yaw, b.Yaw));
    return sum;
}

float TargetInterpolator::TimeToFrame(){
    return timeToFrame;
}



FVector TargetInterpolator::readFromPosition(){
    return from;
}

FVector TargetInterpolator::readToPosition(){
    return target;
}








/** 
 * 
 * 
 * ---- WORLD SPACE SECTION ----
 * 
 * 
 * 
 * man könnte hier noch überlegen ob man start local und worldgleichzeitig baut
 * 
 * 
*/


void TargetInterpolator::overrideStartWorld(FVector targetIn){
    fromWorld = targetIn;
}

void TargetInterpolator::overrideTargetWorld(FVector targetIn){
    targetWorld = targetIn;
    worldtargetSetup = true;
}

/// @brief overrides the starting position of the interpolation in world position
/// if the world target is setup, the local target will be also updated, relative to the actor matrix transform
/// @param newStart new world frame start
/// @param actor actor with rotation and translation
void TargetInterpolator::overrideStartWorldSpeedRelative(FVector newStart, MMatrix &actor){

    MMatrix inverse = actor.jordanInverse();
    if (!worldtargetSetup)
    {
        //translate start to local and override start
        FVector startLocal = inverse * newStart;
        overrideStartSpeedRelative(startLocal);
        return;
    }else{
        //DebugHelper::showScreenMessage("OVERRIDE WORLD START TARGET");
        
        //translate start and target to local and override both, target must be available
        FVector targetLocalNew = inverse * targetWorld;
        overrideTarget(targetLocalNew);
        FVector startLocal = inverse * newStart;
        overrideStartSpeedRelative(startLocal);
        return;
    }
}







/**
 * new bezier interpolation - testing needed
 * is not debugged!
 */
FVector TargetInterpolator::interpolationBezier(float skalar){
    FVector tangente = (from - prev).GetSafeNormal(); //AB = B - A
    float dist = FVector::Dist(from, target);
    float skalDist = dist * 0.25f;
    FVector weight = from + tangente * skalDist;

    if(skalar >= 1.0f){
        prev = weight; //save weight for prev knot tangent
        return target;
    }

    

    return ThreeAnchorBezier(from, weight, target, skalar);
}

FVector TargetInterpolator::ThreeAnchorBezier(
    FVector &a,
    FVector &b,
    FVector &c,
    float skalar
){
    FVector ab = a + skalar * (b - a);
    FVector bc = b + skalar * (c - b);
    FVector abbc = ab + skalar * (bc - ab); // das ding dazwischen
    return abbc;
}