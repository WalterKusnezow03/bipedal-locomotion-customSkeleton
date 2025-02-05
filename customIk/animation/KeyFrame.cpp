// Fill out your copyright notice in the Description page of Project Settings.


#include "p2/entities/customIk/animation/KeyFrame.h"

KeyFrame::KeyFrame(FVector positionIn, float velocityIn)
{
    position = positionIn;
    setVelocity(velocityIn);
    grounded = false;
}

KeyFrame::KeyFrame(FVector positionIn, float velocityIn, bool groundedIn){
    position = positionIn;
    setVelocity(velocityIn);
    grounded = groundedIn;
}

KeyFrame::~KeyFrame()
{
}

void KeyFrame::setVelocity(float velocityIn){
    velocity = velocityIn;
}

FVector KeyFrame::readposition(){
    return position;
}

float KeyFrame::readVelocity(){
    return velocity;
}

bool KeyFrame::mustBeGrounded(){
    return grounded;
}