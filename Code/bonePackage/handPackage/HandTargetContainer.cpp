// Fill out your copyright notice in the Description page of Project Settings.

#include "HandBoneIndexEnum.h"
#include "HandTargetContainer.h"


HandTargetContainer::HandTargetContainer()
{
}

HandTargetContainer::~HandTargetContainer()
{
}


void HandTargetContainer::setup(HandBoneIndexEnum handtypeIn){
    handtype = HandBoneIndexEnum::leftHand;
    if (handtypeIn == HandBoneIndexEnum::rightHand || handtypeIn == HandBoneIndexEnum::leftHand)
    {
        handtype = handtypeIn;
    }
}

HandBoneIndexEnum HandTargetContainer::readHandtype(){
    return handtype;
}

void HandTargetContainer::setOrientation(FRotator &other){
    handOrientation.setRotation(other);
}

void HandTargetContainer::setOrientation(MMatrix &other){
    handOrientation.setRotation(other);
}

/// @brief do not modify
/// @return 
MMatrix &HandTargetContainer::rotationByReference(){
    return handOrientation;
}

void HandTargetContainer::updateTargetWorld(FVector other, HandBoneIndexEnum type){
    targets[type] = other;
}

FVector HandTargetContainer::getTargetWorld(HandBoneIndexEnum type){
    if(targets.find(type) != targets.end()){
        return targets[type];
    }
    return FVector(0, 0, 0);
}
