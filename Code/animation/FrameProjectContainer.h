// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "p2/entities/customIk/MMatrix.h"

/**
 * will store the world, actor matrix (with orientation) to bring a frame from world to cocala space
 * velocity and look direction for moving projected frames to the future
 * 
 * can store the world hit result and offset made during projection (For later gravity adjustment on actor)
 */
class P2_API FrameProjectContainer
{
public:
	FrameProjectContainer();
	~FrameProjectContainer();

	void setup(
		UWorld *world, 
		MMatrix &currentActorMatrixTemporary, 
		float velocity, 
		FVector lookDir, 
		float lowerLimitToClimb,
		float maxHeightForProjectionIn
	);

	UWorld *getWorld();
	MMatrix &actorMatrix();
	FVector getLookDir();
	float getVelocity();

	void updateWorldHitAndOffset(FVector &worldHitIn, FVector &offsetFromOriginalIn);
	FVector getWorldHit();
	FVector getOffsetFromOriginal();

	bool exceedsMaxHeight(FVector &offsetFromOriginal);
	bool exceedsMaxHeight();

	bool startClimb();
	bool startClimbingAndNoExceedingMaxHeight();

private:

	class UWorld *world;
	class MMatrix actorMatrixCopy;
	
	FVector offsetMade;
	FVector worldHit;
	float velocity;
	FVector lookdir;

	FVector offsetFromOriginal;

	float maxHeightForProjection;
	float minHeightStartClimb;
};
