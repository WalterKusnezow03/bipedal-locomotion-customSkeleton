// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"
#include "p2/entities/customIk/bonePackage/FabrikBone.h"

FabrikBone::FabrikBone()
{   
    chain.push_back(FVector(0,0,0));
    chain.push_back(FVector(0,0,200));
    chain.push_back(FVector(0,0,400));
    chain.push_back(FVector(0,0,600));
    chain.push_back(FVector(0,0,800));
}

FabrikBone::FabrikBone(int pieces, float distance){
    pieces = std::abs(pieces);
    distance = std::abs(distance);

    for(int i = 0; i < pieces; i++){
        chain.push_back(FVector(0,0,i*distance));
    }
}



FabrikBone::~FabrikBone()
{
}



void FabrikBone::moveEndToTarget(UWorld *world, FVector localTarget, MMatrix &offset, float displayTime){
    fabrik(chain, localTarget);

    std::vector<FVector> worldPositions;
    for(int i = 0; i < chain.size(); i++){
        FVector inWorld = offset * chain[i];
        worldPositions.push_back(inWorld);
    }
    drawChain(worldPositions, world, displayTime);
}



void FabrikBone::fabrik(std::vector<FVector>&joints, FVector &target){

    //muss noch geändert werden
    FVector basePosition(0,0,0);
    int tolerance = 10;
    int maxIterations = 100;



    int n = joints.size();
    std::vector<float> lengths;

    for (int i = 0; i < n - 1; ++i) {
        float length = FVector::Dist(joints[i + 1], joints[i]);
        lengths.push_back(length);
    }

    

    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        // Forward Phase
        joints[n - 1] = target;
        for (int i = n - 2; i >= 0; --i) {
            FVector dir = (joints[i] - joints[i + 1]).GetSafeNormal();
            joints[i] = joints[i + 1] + dir * lengths[i];
        }

        // Backward Phase
        joints[0] = basePosition; // Fixiere das Basisgelenk
        for (int i = 1; i < n; ++i) {
            FVector dir = (joints[i] - joints[i - 1]).GetSafeNormal();
            joints[i] = joints[i - 1] + dir * lengths[i - 1];
        }

        // Prüfe auf Konvergenz (?lol)
        float distance = std::abs(FVector::Dist(joints[n-1], target));
        if (distance < tolerance) {
            
            break;
        }
    }
}




void FabrikBone::drawChain(std::vector<FVector> &positions, UWorld *world, float displayTime){
    
    for(int i = 1; i < positions.size(); i++){
        FVector &prev = positions[i-1];
        FVector &current = positions[i];
        FColor color = (i % 2 == 0) ? FColor::Red : FColor::Blue;
        DebugHelper::showLineBetween(world, prev, current, color, displayTime);
    }
}




