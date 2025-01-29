// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include <list>
#include "carriedItem.generated.h"

UCLASS()
class P2_API AcarriedItem : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AcarriedItem();

	void pickup(UCameraComponent *cameraIn);
	void pickupBot(AActor *actorIn); //pickup for bot!
	virtual void drop();
	bool isPickedupByPlayer();
	void showItem(bool show);

	virtual void leftMouseDown();
	virtual void leftMouseUp();

	bool isActive();


	virtual FVector leftHandLocation();
	virtual FVector rightHandLocation();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/// @brief will save whether the item is shown (selected or not.)
	bool isVisible;

	/// @brief shows or hides a child actor component if not nullptr
	/// @param component 
	/// @param show 
	void showChildActor(UChildActorComponent *component, bool show);

	//follow
	class UCameraComponent *cameraPointer; // why class, idk, just keep it like that
	class AActor *botPointer;
	void followPlayer();
	FVector offset;

	///offset vector, overriden by weapon script
	virtual FVector getOffsetVector();
	

	void enableCollider(bool enable);

	void showScreenMessage(FString s);

	
	template <typename T>
	void findAllOfType(AActor &a, std::list<T *> &items);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
