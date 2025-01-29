// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class P2_API GravityInterpolator
{
public:
	GravityInterpolator();
	~GravityInterpolator();

	FVector interpolate(FVector &currentPos, float DeltaTime);
	FVector interpolate(FVector &currentPos, FVector &velocity, float DeltaTime);
	void resetVelocity();

	void updateGroundPosition(FVector groundIn);
	bool groundReachedFlag();

private:
	FVector groundPosition;
	float epsilonGroundReached = 0.5f;

	bool reachedFlag = false;

	FVector gravityVector();
	bool groundReached(FVector &compare);
	

	float gravityA = -981.0f; //9.81 meter
	FVector velocityVector;
};
