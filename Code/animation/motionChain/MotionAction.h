// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

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

	void setLocation(FVector &pos);
	void setRotation(FRotator &rotator);
	void setLocationAndRotation(FVector &pos, FRotator &rotator);

private:
	FVector targetLocation;
	FRotator targetRotation;
};
