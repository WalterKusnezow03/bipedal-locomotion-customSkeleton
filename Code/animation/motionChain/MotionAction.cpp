// Fill out your copyright notice in the Description page of Project Settings.


#include "p2/entities/customIk/animation/motionChain/MotionAction.h"

MotionAction::MotionAction()
{
}

MotionAction::MotionAction(FVector &a, FRotator &b){
    targetRotation = b;
    targetLocation = a;
    targetIsSplit2Arms = false;
}

MotionAction::MotionAction(const MotionAction &other){
    *this = other;
}


MotionAction& MotionAction::operator=(const MotionAction &other){
    if(this == &other){
        return *this;
    }
    targetRotation = other.targetRotation;
    targetLocation = other.targetLocation;
    targetIsSplit2Arms = other.targetIsSplit2Arms;
    return *this;
}

MotionAction::~MotionAction()
{
}






FVector MotionAction::copyPosition(){
    return targetLocation;
}

FRotator MotionAction::copyRotation(){
    return targetRotation;
}

MMatrix MotionAction::copyRotationAsMMatrix(){
    return MMatrix::createRotatorFrom(targetRotation);
}

void MotionAction::setLocation(FVector &pos){
    targetLocation = pos;
}

void MotionAction::setRotation(FRotator &rotator){
    targetRotation = rotator;
}

void MotionAction::setLocationAndRotation(FVector &pos, FRotator &rotator){
    setLocation(pos);
    setRotation(rotator);
}

void MotionAction::setLocationAndRotation(MMatrix &transformMat){
    FVector pos = transformMat.getTranslation();
    FRotator rotator = transformMat.extractRotator();
    setLocationAndRotation(pos, rotator);
}



void MotionAction::copyPositionSymetricalOnYZPane(
    FVector &outLeft,
    FVector &outRight
){
    outLeft = copyPosition();
    outRight = copyPosition();
    outLeft.Y *= -1.0f;

}


void MotionAction::setLocalFrame2ArmsSeperate(bool in){
    targetIsSplit2Arms = in;
}

bool MotionAction::isSetToLocalFrame2armSeperate(){
    return targetIsSplit2Arms;
}