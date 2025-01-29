// Fill out your copyright notice in the Description page of Project Settings.


#include "p2/entities/customIk/animation/GravityInterpolator.h"

GravityInterpolator::GravityInterpolator()
{
    resetVelocity();
}



GravityInterpolator::~GravityInterpolator()
{
}

/// @brief returns a vector which needs to be added to a position to have gravity based on the
/// passed parameters!
/// @param currentPos 
/// @param velocity 
/// @param DeltaTime 
/// @return 
FVector GravityInterpolator::interpolate(FVector &currentPos, FVector &velocity, float DeltaTime){
    if(groundReached(currentPos)){
        resetVelocity();
        return FVector(0,0,0);
    }

    //x(t) = x0 + v0t + 1/2 at^2
    FVector accelaration = gravityVector();
    FVector accelerationAt2 = 0.5f * accelaration * DeltaTime * DeltaTime;
    FVector interpolatedPos = currentPos + velocity * DeltaTime + accelerationAt2;

    //v(t) = v0 + at
    FVector v0New = (interpolatedPos - currentPos) / DeltaTime;
    //velocity + accelaration * DeltaTime; //hier stimmt noch was nicht
    velocityVector = v0New;

    //AB = B - A
    return interpolatedPos - currentPos;

    //return interpolatedPos;
}

/// @brief returns a vector which needs to be added to a position to have gravity based on the
/// passed parameters!
/// @param currentPos 
/// @param DeltaTime 
FVector GravityInterpolator::interpolate(FVector &currentPos, float DeltaTime){
    return interpolate(currentPos, velocityVector, DeltaTime);
}

bool GravityInterpolator::groundReached(FVector &compare){
    if(reachedFlag){
        return true;
    }

    //bool distanceReached = FVector::Dist(groundPosition, compare) <= epsilonGroundReached;
    bool belowGround = groundPosition.Z > compare.Z;

    if(belowGround){
        reachedFlag = true;
        return true;
    }
    return false;
}

bool GravityInterpolator::groundReachedFlag(){
    return reachedFlag;
}

void GravityInterpolator::resetVelocity(){
    velocityVector = FVector(0.0f, 0.0f, 0.0f);
}

FVector GravityInterpolator::gravityVector(){
    return FVector(0.0f, 0.0f, gravityA);
}


void GravityInterpolator::updateGroundPosition(FVector groundIn){
    groundPosition = groundIn;
    reachedFlag = false;
}