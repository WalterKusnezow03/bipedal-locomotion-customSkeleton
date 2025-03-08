#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
UENUM()
enum class ArmMotionStates
{
	none,
    withDrawItem, //needed for mving hands somewhere
    holsterItem, //needed for moving item somewhere
    handsFollowItem, //needed for arms holding something
    kontaktStellung,
    running,
    wingsuitOpen
};