// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class P2_API KeyFrame
{
public:
	KeyFrame(FVector positionIn, float velocityIn);
	KeyFrame(FVector positionIn, float velocityIn, bool groundedIn);
	~KeyFrame();

	FVector readposition();
	float readVelocity();
	bool mustBeGrounded();

	void setVelocity(float velocityIn);

private:
	FVector position;
	float velocity;
	bool grounded = false;
};
