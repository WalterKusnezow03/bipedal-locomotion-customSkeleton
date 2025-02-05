// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "p2/entities/customIk/MMatrix.h"

/**
 * 
 */
class P2_API MotionAction
{
public:
	MotionAction();
	MotionAction(FVector &a, FRotator &b);
	~MotionAction();

	FVector copyPosition();
	FRotator copyRotation();
	MMatrix copyRotationAsMMatrix();

	void setLocation(FVector &pos);
	void setRotation(FRotator &rotator);
	void setLocationAndRotation(FVector &pos, FRotator &rotator);

private:
	FVector targetLocation;
	FRotator targetRotation;
};
