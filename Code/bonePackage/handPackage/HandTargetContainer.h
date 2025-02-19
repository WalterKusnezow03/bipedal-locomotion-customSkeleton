// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <map>
#include "p2/entities/customIk/MMatrix.h"
#include "HandBoneIndexEnum.h"

/**
 * saves the target rotation of the hand and finger positions in world space
 */
class P2_API HandTargetContainer
{
public:
	HandTargetContainer();
	~HandTargetContainer();

	void setup(HandBoneIndexEnum handtypeIn);

	void setOrientation(FRotator &other);
	void setOrientation(MMatrix &other);

	void updateTargetWorld(FVector other, HandBoneIndexEnum type);

	FVector getTargetWorld(HandBoneIndexEnum type);

	HandBoneIndexEnum readHandtype();

	MMatrix &rotationByReference();

private:
	MMatrix handOrientation;
	HandBoneIndexEnum handtype;

	std::map<HandBoneIndexEnum, FVector> targets;
};
