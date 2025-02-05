// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class P2_API FabrikBone
{
public:
	FabrikBone();
	FabrikBone(int pieces, float distance);
	~FabrikBone();

	void moveEndToTarget(UWorld *world, FVector localTarget, MMatrix &offset, float displayTime);

private:
	void drawChain(std::vector<FVector> &positions, UWorld *world, float displayTime);
	void fabrik(std::vector<FVector>&joints, FVector &target);
	std::vector<FVector> chain;
};
