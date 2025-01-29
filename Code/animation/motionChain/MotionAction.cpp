// Fill out your copyright notice in the Description page of Project Settings.


#include "p2/entities/customIk/animation/motionChain/MotionAction.h"

MotionAction::MotionAction()
{
}

MotionAction::MotionAction(FVector &a, FRotator &b){
    targetRotation = b;
    targetLocation = a;
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