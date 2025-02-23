// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "p2/entities/customIk/MMatrix.h"

/**
 * will hold 3 matricies to crate an arm or an leg
 * 
 * the default look direction an bone is towards the positive X axis, all angles will
 * be measured to the positive x axis, the initial look dir will be to the positive x axis!
 * 
 * 
 * 
 * CAUTION!
 * x is forward, default look dir is foward X,
 * y is right side by default! remember!
 * 
 */
class P2_API TwoBone
{
public:
	TwoBone();
	TwoBone(const TwoBone &other); //copy constructor für push back notwending
	~TwoBone();

	TwoBone& operator=(const TwoBone &other);


	FVector endLimbWorldLocation();
	FVector startRelativeToEnd_Initial();

	void setupBones(float completeDistance);
	void setupBones(float distanceHipKnee, float distanceKneeFoot);
	void setAsArm();
	bool isArmBone();

	void build(
		UWorld *world, 
		MMatrix &offsetAndRotation, 
		MMatrix &endEffector, 
		FColor color, 
		float displayTime
	);



	
	void rotateEndToTarget(FVector &vec, FVector &weight);
	void rotateStartToTarget(FVector &vec, FVector &weight);

	void rotateStartToTargetAndBuild(
		UWorld *world,
		FVector &vec,
		FVector &weight,
		MMatrix &offsetAndRotation,
		MMatrix &translationOfactor,
		FColor color,
		float displayTime
	);

	//new with foot pos update
	void rotateEndToTargetAndBuild(
		UWorld *world,
		FVector &vec,
		FVector &weight,
		MMatrix &offsetAndRotation,
		MMatrix &endEffectorWorldToUpdate,
		FColor color,
		float displayTime
	);



	//New sction for actor attaching
	void attachFirtsLimb(AActor &actorToAttach);
	void attachSecondLimb(AActor &actorToAttach);
	void attachThirdLimb(AActor &actorToAttach);

	FVector copyLatestStartLimbPosition();
	FVector copyLatestEndLimbPosition();


	void resetAllRotations();

private:
	UPROPERTY()
	class AActor *hipLimbPointer = nullptr;
	UPROPERTY()
	class AActor *kneeLimbPointer = nullptr;
	UPROPERTY()
	class AActor *footLimbPointer = nullptr;
	bool actorIsAlreadAttached(AActor &actor);

	void disableLimbCollision(AActor &actor);

private:
	bool isArmFlag = false;

	float createEthaFromDistance(float distance);
	float angleFromEtha(float etha);
	float createHipAngle(float angle);
	float createKneeAngle(float angle);
	void normalizeTarget(FVector &target);

	void createEthaPitchAnglesFor(
		float distanceTotarget, 
		float &firstOuput, 
		float &secondOutput, 
		bool forwardIK,
		MMatrix &middle,
    	MMatrix &end
	);

	bool flipAngleForBoneNeeded(FVector &target, FVector &weight, float hipAngle);

	//calculating angles
	float yawAngleTo(FVector &localTarget);
	float pitchAngleTo(FVector &localTarget);
	float rollAngleTo(FVector &localTarget);

	float pitchAngleToInitialLookDirOfBone(FVector &localTarget);
	float pitchAngleToInitiaToUpDirOfBone(FVector &localTarget);

	

	float flipRotation(float aX, float aY, float oX, float oY);

	/// @brief hip or shoulder
	MMatrix hip;
	
	/// @brief knee or ellbow
	MMatrix knee;
	
	/// @brief foot or hand
	MMatrix foot;

	///@brief foot tip or finger tips end
	FVector toFootTip;

	///@brief will save the total length of the bone for later usuage, dont change, only constructor
	float totalBoneLengthCopy = 0.0f;

	void getMatricies(std::vector<MMatrix*> &dataout, FVector &outVector);
	
	void setEtha(float etha, float legPitchThetaRadian);





	float clampEtha(float etha);

	void rotateEndToTarget(
		FVector &vec,
		FVector &weight,
		MMatrix &start,
		MMatrix &middle,
		MMatrix &end,
		bool debugForwardIk
	);
	



	//copy positions for mesh adjustment
	MMatrix hipCopy;
	MMatrix kneeCopy;
	MMatrix footCopy;

	void updateLimb(AActor *limb, MMatrix &matrixRotator, FVector location);


	MMatrix buildWithOutput(
		UWorld *world,
		MMatrix &offsetAndRotation,
		FColor color,
		float displayTime,
		std::vector<MMatrix *> &matrizen, // must not be empty
		std::vector<AActor *> &attachedBones,
		bool forward
	);


	
	
	//new
	MMatrix buildWithOutput(
		UWorld *world,
		MMatrix &offsetAndRotation,
		FColor color,
		float displayTime,
		std::vector<MMatrix *> &matrizen // must not be empty
	);

	
	void copyCurrentMatricies();

	bool pitchTooLow(float pitchAngle);
};
