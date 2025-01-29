// Fill out your copyright notice in the Description page of Project Settings.


#include "p2/entities/customIk/bonePackage/FabrikBone.h"
#include "Kismet/KismetMathLibrary.h"
#include <cmath>
#include "p2/entities/customIk/animation/KeyFrameAnimation.h"
#include "p2/entities/customIk/animation/DoubleKeyFrameAnimation.h"
#include "p2/gameStart/assetManager.h"
#include "p2/_world/worldLevel.h"
#include "p2/entityManager/EntityManager.h"
#include "p2/entities/customIk/IkActor.h"
#include "p2/entities/customIk/bonePackage/TwoBone.h"

// Sets default values
AIkActor::AIkActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AIkActor::BeginPlay()
{
	Super::BeginPlay();


	//init offset for now
	FVector offset(1000, -1000, 200);
	hipController.SetLocation(offset);

	
	/*
	//debug matrix inverse bestimmen (klappt):
	MMatrix current = currentTransform();
	MMatrix invers = current.jordanInverse(); // current.adjunktInverse();
	MMatrix identitity = current * invers;

	FString message1 = FString::Printf(TEXT("debug matrix inverse matrix: "));
	message1 += invers.asString();
	DebugHelper::logMessage(message1);

	FString message = FString::Printf(TEXT("debug matrix Sollte einheits matrix ergeben: "));
	message += identitity.asString();
	DebugHelper::logMessage(message);
	*/

	weaponPointer = nullptr;
	hipController.SetControllerState(BoneControllerStates::locomotion);
	getWeaponOnStart();
	hipController.attachCarriedItem(weaponPointer);


	//testing rotation
	hipController.yawRotate(60);



	//testing fabrik
	/*
	float debugDisplayTime = 100.0f;
	FabrikBone debugFabrikBone;
	debugFabrikBone.moveEndToTarget(GetWorld(), FVector(200,-300,200), ownLocation, debugDisplayTime);
	*/



	//extract angles
	/*
	MMatrix rotTest;
	rotTest.yawRadAdd(MMatrix::degToRadian(90));
	rotTest.yawRadAdd(MMatrix::degToRadian(25));
	rotTest.pitchRadAdd(MMatrix::degToRadian(45));
	rotTest.pitchRadAdd(MMatrix::degToRadian(-10));
	FRotator r = rotTest.extractRotator();
	FString string = FString::Printf(
		TEXT("ROTATION DEBUG x %f ; y %f; z %f"), 
		r.Roll,
		r.Pitch,
		r.Yaw
	);
	DebugHelper::logMessage(string);
	*/


	// debug testing meshes
	float legScaleCM = 150.0f;
	float armScaleCM = 100.0f;
	float legHalfScale = legScaleCM / 2.0f;
	float armHalfScale = armScaleCM / 2.0f;

	int sizeX = 10;
	int sizeY = 10;
	int offY = sizeY / 2;
	offY = 0;

	AActor *oberschenkel = createLimbPivotAtTop(sizeX, sizeY, legHalfScale, offY);
	AActor *unterschenkel = createLimbPivotAtTop(sizeX, sizeY, legHalfScale, offY);
	hipController.attachLimbMeshes(oberschenkel, unterschenkel, 1); //foot 1 debug
	
	AActor *oberschenkel_1 = createLimbPivotAtTop(sizeX, sizeY, legHalfScale, -offY);
	AActor *unterschenkel_1 = createLimbPivotAtTop(sizeX, sizeY, legHalfScale, -offY);
	hipController.attachLimbMeshes(oberschenkel_1, unterschenkel_1, 2); //foot 2 debug

	
	AActor *oberarm = createLimbPivotAtTop(sizeX, sizeY, armHalfScale, -offY);
	AActor *unterarm = createLimbPivotAtTop(sizeX, sizeY, armHalfScale, -offY);
	hipController.attachLimbMeshes(oberarm, unterarm, 3); //hand 1 debug
	

	//torso
	/**
	 * torso wird jetzt erstmal auch hier erstellt
	 * 
	 * die create limb methode usw muss irgendwann entweder durch meshes
	 * aus den assets ersetzt werden
	 * oder eine eigene klasse existieren die diese detailierter
	 * erstellen kann!
	 */
	AActor *torsoMesh = createLimbPivotAtTop(sizeX, sizeY * 4, -armScaleCM, -sizeY * 2.0f);
	hipController.attachTorso(torsoMesh);


	//holding weapon
	AActor *oberarm_1 = createLimbPivotAtTop(sizeX, sizeY, armHalfScale, offY);
	AActor *unterarm_1 = createLimbPivotAtTop(sizeX, sizeY, armHalfScale, offY);
	hipController.attachLimbMeshes(oberarm_1, unterarm_1, 4); //hand 2 debug
	
}

// Called every frame
void AIkActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	hipController.Tick(DeltaTime, GetWorld());
	SetActorLocation(hipController.GetLocation());
	//rotation muss auch noch kopiert werden
}

/// @brief look at a location
/// @param TargetLocation target to look at
void AIkActor::LookAt(FVector TargetLocation) 
{
	hipController.LookAt(TargetLocation);
}

//custom set actor location method for bone ik
void AIkActor::SetLocation(FVector &location){
	hipController.SetLocation(location);
}



/// @brief performs a raycast from a start, into a direction, with a max length in this method
/// @param Start start position
/// @param dir direction of interest
/// @param outputHit output hit if any hit happened
/// @return bool anything was hit
bool AIkActor::performRaycast(FVector &Start, FVector &dir, FVector &outputHit) 
{

	float scaleOfVector = 200000; //some random value for now
	FVector End = Start + dir * scaleOfVector; // gx = A + r (B - A)

	// Perform the raycast
	FHitResult HitResult;

	FCollisionQueryParams ignoreParams;
	/*
	CAUTION: TEAMS PARAM NOT IMPLEMETED YET!

	//params to avoid hitting any other entities
	if(EntityManager *e = worldLevel::entityManager()){
		//ignoreParams = e->getIgnoredRaycastParams(); //example for getting all
		ignoreParams = e->getIgnoredRaycastParams(getTeam());
	}*/
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);
	



	//anything was hit
	if (bHit){
		outputHit = HitResult.ImpactPoint; //write impactpoint to output

		
		
		//DebugHelper::showScreenMessage("RAYCAST HIT");

		return true;
	}

	return false;
}

void AIkActor::getWeaponOnStart(){
	EntityManager *e = worldLevel::entityManager();
    if (e != nullptr)
    {

        //testing new helper (works as expected)
        weaponSetupHelper *setuphelper = new weaponSetupHelper();
        setuphelper->setWeaponTypeToCreate(weaponEnum::pistol);

        Aweapon *w = e->spawnAweapon(GetWorld(), setuphelper);
		//showScreenMessage("begin weapon");
		if (w != nullptr){
			//showScreenMessage("human pickup weapon");
			
			//kein pickup erstmal, managed by bone controller
			//w->pickupBot(this);

            //save pointer
            weaponPointer = w;
        }

        delete setuphelper; //immer löschen nicht vergessen!
        setuphelper = nullptr;
    }
}









/**
 * 
 * 
 * ---- new section for limb creation ----
 * 
 * 
 */

/// @brief creates a cube of x,y,height size
/// @param x x size
/// @param y y size
/// @param height height size
/// @param offsetY offset on y
/// @return new mesh actor
AActor *AIkActor::createLimbPivotAtTop(int x, int y, int height, int offsetY){

	height *= -1; //orient downwardss
	/**
	 * DEBUG CREATE FOLLOW LIMBS
	 */
	UMaterial *material = nullptr;
	assetManager *assetManagerPointer = assetManager::instance();
	if(assetManagerPointer != nullptr){
		material = assetManagerPointer->findMaterial(materialEnum::wallMaterial);
	}

	EntityManager *entitymanagerPointer = worldLevel::entityManager();
	if(entitymanagerPointer != nullptr){
		FVector location(0, 0, 0);
		AcustomMeshActor *oberschenkel = entitymanagerPointer->spawnAcustomMeshActor(GetWorld(), location);
		if(oberschenkel != nullptr){
			//int width = 10;
			//int height = -(legScaleCM / 2);

			FVector a(0,offsetY,0);
			FVector b(x,offsetY,0);
			FVector c(x,y + offsetY,0);
			FVector d(0, y + offsetY,0);
			FVector at(0,offsetY,height);
			FVector bt(x,offsetY,height);
			FVector ct(x,y + offsetY,height);
			FVector dt(0,y + offsetY,height);
			oberschenkel->createCube(
				a,b,c,d,at,bt,ct,dt,
				material
			);
			return oberschenkel;
		}
	}
	return nullptr;
}




