// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "p2/entities/customIk/bonePackage/TwoBone.h"
#include "p2/weapon/carriedItem.h"
#include "HandTargetContainer.h"
#include "HandBoneIndexEnum.h"
#include "p2/entities/customIk/MMatrix.h"

/**
 * 
 */
class P2_API HandController
{
public:
	HandController();
	HandController(HandBoneIndexEnum handIndex);
	HandController(HandController &other);
	~HandController();
	HandController &operator=(HandController &other);

	void setTranslation(FVector vec);
	void setRotation(MMatrix &other);
	void setRotation(FRotator &other);

	void Tick(float DeltaTime, UWorld *worldin);
	void Tick(float DeltaTime, UWorld *worldin, FVector &newLocation);
	void Tick(float DeltaTime, UWorld *worldin, AcarriedItem *item);
	void Tick(float DeltaTime, UWorld *worldin, FVector &baseLocation, AcarriedItem *item);

	void Tick(float DeltaTime, UWorld *worldin, FVector &location, MMatrix &rotationLook, AcarriedItem *item);

	void attachLimbMeshes(AActor *top, AActor *bottom, HandBoneIndexEnum type);

private:
	class UWorld *world = nullptr;
	UWorld *GetWorld();

	HandBoneIndexEnum handIndexSaved = HandBoneIndexEnum::leftHand;

	class HandTargetContainer fingerTargetContainer;

	class MMatrix ownOrientation;
	class MMatrix ownLocation;

	float handWidthCm = 4.0f; //in cm
	int fingerScaleCm = 6;
	void setupBones();

	class TwoBone thumb;
	class TwoBone finger1;
	class TwoBone finger2;
	class TwoBone finger3;
	class TwoBone finger4;

	class MMatrix thumb_start;
	class MMatrix finger1_start;
	class MMatrix finger2_start;
	class MMatrix finger3_start;
	class MMatrix finger4_start;

	class MMatrix thumb_endEffector;
	class MMatrix finger1_endEffector;
	class MMatrix finger2_endEffector;
	class MMatrix finger3_endEffector;
	class MMatrix finger4_endEffector;



	MMatrix *endEffectorByIndex(HandBoneIndexEnum index);
	TwoBone *fingerByIndex(HandBoneIndexEnum index);

	MMatrix currentTransform();
	MMatrix currentTransform(HandBoneIndexEnum index);
	MMatrix offsetMatrix(HandBoneIndexEnum index);

	FVector weightByIndex(HandBoneIndexEnum index);

	void TickLimbNone(HandBoneIndexEnum limbIndex, float DeltaTime);


	void moveBoneAndSnapEndEffectorToTarget(
		HandBoneIndexEnum index,
		float DeltaTime,
		FVector targetWorld,
		FVector weight);



	void DebugdrawHandToFingerStart(float DeltaTime);
	void DebugdrawHandToFingerStart(float DeltaTime, HandBoneIndexEnum index);

	bool isFinger(HandBoneIndexEnum type);
};
