// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "p2/entities/customIk/MMatrix.h"

/**
 * 
 */
class P2_API FrameProjectContainer
{
public:
	FrameProjectContainer();
	~FrameProjectContainer();

	void setup(UWorld *world, MMatrix &currentActorMatrixTemporary, float velocity, FVector lookDir);

	UWorld *getWorld();
	MMatrix &actorMatrix();
	FVector getLookDir();
	float getVelocity();

	void updateWorldHitAndOffset(FVector &worldHitIn, FVector &offsetFromOriginalIn);
	FVector getWorldHit();
	FVector getOffsetFromOriginal();


private:

	class UWorld *world;
	class MMatrix actorMatrixCopy;
	
	FVector offsetMade;
	FVector worldHit;
	float velocity;
	FVector lookdir;

	FVector offsetFromOriginal;
};
