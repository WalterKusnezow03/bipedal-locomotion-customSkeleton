// Fill out your copyright notice in the Description page of Project Settings.


#include "HandController.h"
#include "p2/weapon/carriedItem.h"
#include "HandBoneIndexEnum.h"
#include "p2/entities/customIk/MMatrix.h"

HandController::HandController()
{
    fingerScaleCm = 10;
    handIndexSaved = HandBoneIndexEnum::leftHand;
    setupBones();
    fingerTargetContainer.setup(handIndexSaved);
}

HandController::HandController(HandBoneIndexEnum handindex, int fingerScaleIn){
    fingerScaleCm = std::abs(fingerScaleIn);
    if(fingerScaleCm == 0){
        fingerScaleCm = 1;
    }

    handIndexSaved = HandBoneIndexEnum::leftHand;
    if (handindex == HandBoneIndexEnum::leftHand || handindex == HandBoneIndexEnum::rightHand)
    {
        handIndexSaved = handindex;
    }

    setupBones();
    fingerTargetContainer.setup(handIndexSaved);
}

HandController::HandController(HandController &other){
    *this = other;
}

HandController &HandController::operator=(HandController &other){
    if(this == &other){
        return *this;
    }
    handIndexSaved = other.handIndexSaved;

    thumb = other.thumb;
    finger1 = other.finger1;
    finger2 = other.finger2;
    finger3 = other.finger3;
    finger4 = other.finger4;

    thumb_start = other.thumb_start;
    finger1_start = other.finger1_start;
    finger2_start = other.finger2_start;
    finger3_start = other.finger3_start;
    finger4_start = other.finger4_start;

    thumb_endEffector = other.thumb_endEffector;
    finger1_endEffector = other.finger1_endEffector;
    finger2_endEffector = other.finger2_endEffector;
    finger3_endEffector = other.finger3_endEffector;
    finger4_endEffector = other.finger4_endEffector;

    ownOrientation = other.ownOrientation;
    ownLocation = other.ownLocation;

    fingerScaleCm = other.fingerScaleCm;

    fingerTargetContainer.setup(handIndexSaved);

    return *this;
}

HandController::~HandController()
{
}

int HandController::fingerScale(HandBoneIndexEnum type){
    if(type == HandBoneIndexEnum::thumb){
        return fingerScaleCm * 0.8f;
    }
    return fingerScaleCm;
}

void HandController::setTranslation(FVector vec){
    ownLocation.setTranslation(vec);
}

void HandController::setRotation(MMatrix &other){
    ownOrientation.setRotation(other);
}

void HandController::setRotation(FRotator &other){
    ownOrientation.setRotation(other);
}

void HandController::setupBones(){

    /**
     * x is forward, default look dir is foward X,
     * y is right side by default! remember!
     */

    //set up all offsets based on right and left hand
    float halfWidth = handWidthCm / 2.0f;
    float quaterWidth = handWidthCm / 4.0f;

    if(handIndexSaved == HandBoneIndexEnum::leftHand){
        thumb_start.setTranslation(0, halfWidth, 0);
        finger1_start.setTranslation(handWidthCm, quaterWidth, 0);
        finger2_start.setTranslation(handWidthCm, 0, 0);
        finger3_start.setTranslation(handWidthCm, -1 * quaterWidth, 0);
        finger4_start.setTranslation(handWidthCm, -1 * halfWidth, 0);
    }
    if(handIndexSaved == HandBoneIndexEnum::rightHand){
        thumb_start.setTranslation(0, -1 * halfWidth, 0);
        finger1_start.setTranslation(handWidthCm, -1 * quaterWidth, 0);
        finger2_start.setTranslation(handWidthCm, 0, 0);
        finger3_start.setTranslation(handWidthCm, quaterWidth, 0);
        finger4_start.setTranslation(handWidthCm, halfWidth, 0);
    }



    //set up all bones
    float thumbScale = fingerScale(HandBoneIndexEnum::thumb);
    thumb.setupBones(thumbScale);
    finger1.setupBones(fingerScaleCm);
    finger2.setupBones(fingerScaleCm);
    finger3.setupBones(fingerScaleCm);
    finger4.setupBones(thumbScale);
    return; //DEBUG

    thumb.setAsArm();
    finger1.setAsArm();
    finger2.setAsArm();
    finger3.setAsArm();
    finger4.setAsArm();
}

MMatrix HandController::currentTransform(){
    return ownLocation * ownOrientation; //M = T * R <-- lese richtung --
}

MMatrix HandController::offsetMatrix(HandBoneIndexEnum index){
    if(index == HandBoneIndexEnum::thumb){
        return thumb_start;
    }
    if(index == HandBoneIndexEnum::finger1){
        return finger1_start;
    }
    if(index == HandBoneIndexEnum::finger2){
        return finger2_start;
    }
    if(index == HandBoneIndexEnum::finger3){
        return finger3_start;
    }
    if(index == HandBoneIndexEnum::finger4){
        return finger4_start;
    }
    return MMatrix();
}

MMatrix HandController::currentTransform(HandBoneIndexEnum index){
    MMatrix TR = currentTransform();
    MMatrix offset = offsetMatrix(index);
    MMatrix result = TR * offset; //M = T * R * Toff <-- lese richtung --
    return result;
}

TwoBone *HandController::fingerByIndex(HandBoneIndexEnum index){
    if(index == HandBoneIndexEnum::thumb){
        return &thumb;
    }
    if(index == HandBoneIndexEnum::finger1){
        return &finger1;
    }
    if(index == HandBoneIndexEnum::finger2){
        return &finger2;
    }
    if(index == HandBoneIndexEnum::finger3){
        return &finger3;
    }
    if(index == HandBoneIndexEnum::finger4){
        return &finger4;
    }
    return nullptr;
}

MMatrix *HandController::endEffectorByIndex(HandBoneIndexEnum index){
    if(index == HandBoneIndexEnum::thumb){
        return &thumb_endEffector;
    }
    if(index == HandBoneIndexEnum::finger1){
        return &finger1_endEffector;
    }
    if(index == HandBoneIndexEnum::finger2){
        return &finger2_endEffector;
    }
    if(index == HandBoneIndexEnum::finger3){
        return &finger3_endEffector;
    }
    if(index == HandBoneIndexEnum::finger4){
        return &finger4_endEffector;
    }
    return nullptr;
}



/**
 * 
 * --- Tick section ---
 * 
 */
UWorld *HandController::GetWorld(){
    return world;
}

void HandController::Tick(float DeltaTime, UWorld *worldin){
    FVector copyOwn = ownLocation.getTranslation();
    Tick(DeltaTime, worldin, copyOwn);
}


void HandController::Tick(float DeltaTime, UWorld *worldin, FVector &newLocation){
    //save world pointer!
    world = worldin;
    
    ownLocation.setTranslation(newLocation);

    //hier dann: build all bones raw!
    TickLimbNone(HandBoneIndexEnum::thumb, DeltaTime);
    TickLimbNone(HandBoneIndexEnum::finger1, DeltaTime);
    TickLimbNone(HandBoneIndexEnum::finger2, DeltaTime);
    TickLimbNone(HandBoneIndexEnum::finger3, DeltaTime);
    TickLimbNone(HandBoneIndexEnum::finger4, DeltaTime);
}

void HandController::TickLimbNone(HandBoneIndexEnum limbIndex, float DeltaTime){
	
	TwoBone *bone = fingerByIndex(limbIndex);
	MMatrix *endEffector = endEffectorByIndex(limbIndex);

	if(bone != nullptr && endEffector != nullptr){

		MMatrix transform = currentTransform(limbIndex);

		bone->build(
			GetWorld(),
			transform,
			*endEffector, // by reference updated
			FColor::Red,
			DeltaTime * 2.0f
		);
	}
}



void HandController::Tick(
    float DeltaTime, 
    UWorld *worldin,
    FVector &location, 
    MMatrix &rotationLook, 
    AcarriedItem *item
){
    ownOrientation.setRotation(rotationLook);
    Tick(DeltaTime, worldin, location, item);
}

void HandController::Tick(float DeltaTime, UWorld *worldin, FVector &newLocation, AcarriedItem *item){
    if(item == nullptr){
        Tick(DeltaTime, worldin, newLocation);
    }
    setTranslation(newLocation);
    world = worldin;

    //debug
    if(false){
        if(handIndexSaved == HandBoneIndexEnum::leftHand){
            return;
        }
    }


    //hier dann: alle bones an das item attachen wenn möglich!
    int size = 5;
    HandBoneIndexEnum fingerIndex[] = {
        HandBoneIndexEnum::thumb,
        HandBoneIndexEnum::finger1,
        HandBoneIndexEnum::finger2,
        HandBoneIndexEnum::finger3,
        HandBoneIndexEnum::finger4
    };

    //load targets
    item->loadFingerTargets(fingerTargetContainer);

    //update rotation
    ownOrientation = ownOrientation * fingerTargetContainer.rotationByReference();
    //setRotation(fingerTargetContainer.rotationByReference());

    if(true){
        for (int i = 0; i < size; i++){
            HandBoneIndexEnum index = fingerIndex[i];
            moveBoneAndSnapEndEffectorToTarget(
                index,
                DeltaTime,
                fingerTargetContainer.getTargetWorld(index),
                weightByIndex(index)
            );
        }
    }
    

    DebugdrawHandToFingerStart(DeltaTime);
}



void HandController::moveBoneAndSnapEndEffectorToTarget(
	HandBoneIndexEnum limbIndex, 
	float DeltaTime, 
	FVector targetWorld,
	FVector weight
){
	weight = weight.GetSafeNormal();

	TwoBone *bone = fingerByIndex(limbIndex);
	MMatrix *end = endEffectorByIndex(limbIndex);
	MMatrix translationActor = currentTransform(limbIndex);
    FColor color = FColor::Red;

    //DebugHelper::showLineBetween(GetWorld(), targetWorld, targetWorld + FVector(0, 0, 100), FColor::Orange);

    if (bone != nullptr && end != nullptr)
	{

		FVector targetLocal = targetWorld;
        translationActor.transformFromWorldToLocalCoordinates(targetLocal);

        bone->rotateEndToTargetAndBuild(
			GetWorld(),
			targetLocal,
			weight,
			translationActor, // hip start with orient
			*end, //ownLocationFoot,  // foot apply positions
			color, 
			DeltaTime * 2.0f
		);
	}
}



FVector HandController::weightByIndex(HandBoneIndexEnum index){
    return FVector(1, 0, 0);
}





void HandController::DebugdrawHandToFingerStart(float DeltaTime){
    DebugdrawHandToFingerStart(DeltaTime, HandBoneIndexEnum::thumb);
    DebugdrawHandToFingerStart(DeltaTime, HandBoneIndexEnum::finger1);
    DebugdrawHandToFingerStart(DeltaTime, HandBoneIndexEnum::finger2);
    DebugdrawHandToFingerStart(DeltaTime, HandBoneIndexEnum::finger3);
    DebugdrawHandToFingerStart(DeltaTime, HandBoneIndexEnum::finger4);



    //draw forward
    if(true && DEBUG_DRAW){
        FVector start = currentTransform().getTranslation();
        FVector dir = ownOrientation.lookDirXForward();
        DebugHelper::showLineBetween(
            GetWorld(),
            start,
            start + dir * 30.0f,
            FColor::Purple,
            DeltaTime * 1.1f
        );
    }
    
}

void HandController::DebugdrawHandToFingerStart(float DeltaTime, HandBoneIndexEnum index){
    if(DEBUG_DRAW){
        MMatrix a = currentTransform(index);
        MMatrix b = currentTransform();
        DebugHelper::showLineBetween(
            GetWorld(),
            a.getTranslation(),
            b.getTranslation(),
            FColor::Orange,
            DeltaTime * 1.1f
        );
    }
    
}








void HandController::attachLimbMeshes(AActor *top, AActor *bottom, HandBoneIndexEnum type){
    if(isFinger(type)){
        TwoBone *bone = fingerByIndex(type);
        if(bone != nullptr){
            if(top != nullptr){
                bone->attachFirtsLimb(*top);
            }
            if(bottom != nullptr){
                bone->attachSecondLimb(*bottom);
            }
        }
    }
}

bool HandController::isFinger(HandBoneIndexEnum type){
    return type == HandBoneIndexEnum::thumb ||
           type == HandBoneIndexEnum::finger1 ||
           type == HandBoneIndexEnum::finger2 ||
           type == HandBoneIndexEnum::finger3 ||
           type == HandBoneIndexEnum::finger4;
}