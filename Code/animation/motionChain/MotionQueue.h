// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MotionAction.h"
#include "p2/entities/customIk/animation/TargetInterpolator.h"

/**
 * stores states based on the arm motion state enum
 * change states to interpolate between them
 */
class P2_API MotionQueue
{
public:
	MotionQueue();
	~MotionQueue();

	bool isTransitioning();
	void addTarget(ArmMotionStates state, MotionAction action);
	void updateState(ArmMotionStates state);
	void Tick(MMatrix &transform, TwoBone &bone, AcarriedItem &item, float DeltaTime);

	void Tick(
		MMatrix &transform,
		MMatrix &transformLeftArm,
		MMatrix &transformRightArm,
		MMatrix &endEffectorRight,
    	MMatrix &endEffectorLeft,
		TwoBone &leftArm,
		TwoBone &rightArm,
		AcarriedItem *item,
		float DeltaTime
	);

private:
	ArmMotionStates currentState;
	std::map<ArmMotionStates, MotionAction> statesMap;

	class TargetInterpolator interpolator;

	bool transitioning = false;

	void moveBoneAndSnapEndEffectorToTarget(
		float DeltaTime,
		FVector targetLocal,
		FVector weight,
		MMatrix &translationActor,
		MMatrix &endEffector,
		TwoBone &bone,
		UWorld *world
	);

	void moveAndBuildBone(
		float DeltaTime,
		MMatrix &translationActor, // shoulder start
		MMatrix &endEffector,
		TwoBone &bone,
		UWorld *world
	);

	bool handsAtItem();
};
