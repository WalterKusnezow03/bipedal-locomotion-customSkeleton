// Fill out your copyright notice in the Description page of Project Settings.

#include <algorithm>
#include "p2/entities/customIk/bonePackage/TwoBone.h"
#include "p2/entities/customIk/MMatrix.h"

TwoBone::TwoBone()
{
    /**
     * x is forward, default look dir is foward X,
     * y is right side by default! remember!
     */
    hipLimbPointer = nullptr;
	kneeLimbPointer = nullptr;
	footLimbPointer = nullptr;
    isArmFlag = false;
}

TwoBone& TwoBone::operator=(const TwoBone &other){
    if(&other == this){
        return *this;
    }
    hipLimbPointer = other.hipLimbPointer;
	kneeLimbPointer = other.kneeLimbPointer;
	footLimbPointer = other.footLimbPointer;

    hip = other.hip;
    knee = other.knee;
    foot = other.foot;
    toFootTip = other.toFootTip;
    isArmFlag = other.isArmFlag;

    return *this;
}

TwoBone::TwoBone(const TwoBone &other){
    if(&other != this){
        *this = other; //nicht kopieren
    }
}


TwoBone::~TwoBone()
{
    hipLimbPointer = nullptr;
	kneeLimbPointer = nullptr;
	footLimbPointer = nullptr;
}

void TwoBone::setAsArm(){
    isArmFlag = true;
}

/// @brief sets up the leg or arm (Bone)
/// @param completeDistance distance in meters
void TwoBone::setupBones(float completeDistance){

    totalBoneLengthCopy = completeDistance;

    float half = completeDistance / 2.0f;
    hip.setTranslation(0, 0, 0); //from to hip is 0,0,0
    knee.setTranslation(0, 0, -half); //from hip to knee
    foot.setTranslation(0, 0, -half); //from knee to foot


    toFootTip = FVector(half / 2, 0, 0); //x is forward for now
}

FVector TwoBone::startRelativeToEnd_Initial(){
    return FVector(0, 0, totalBoneLengthCopy);
}

void TwoBone::setupBones(float distanceHipKnee, float distanceKneeFoot){
    distanceHipKnee = std::abs(distanceHipKnee);
    distanceKneeFoot = std::abs(distanceKneeFoot);
    totalBoneLengthCopy = distanceHipKnee + distanceKneeFoot;
    float half = totalBoneLengthCopy / 2.0f;

    hip.setTranslation(0, 0, 0); //from to hip is 0,0,0
    knee.setTranslation(0, 0, -distanceHipKnee); //from hip to knee
    foot.setTranslation(0, 0, -distanceKneeFoot); //from knee to foot

    toFootTip = FVector(half / 2, 0, 0); //x is forward for now
}



/// @brief normalizes a target to the max radius (total bone length)
/// @param target 
void TwoBone::normalizeTarget(FVector &target){
    //normalize the target location if exceeding the bone lenght
    FVector zeroVec(0, 0, 0);
    float distance = FVector::Distance(zeroVec, target);
    if (distance > totalBoneLengthCopy)
    {
        target = target.GetSafeNormal();
        target *= totalBoneLengthCopy;
    }
}




float TwoBone::clampEtha(float etha){
    if(etha < 0.0f){
        etha = 0.0f;
    }
    if(etha > 1.0f){
        etha = 1.0f;
    }
    return etha;
}

/**
 * 
 ----- ETHA EXPLAINED -----

wenn das skalarprodukt zwier vektoren 0 ergibt sind sie othogonal zu einander
wenn das skalarprodukt zwier vektoren 1 ergibt sind sie paralell zu einander

wenn das bein ausgestreckt, ist der cos(theta) * n = 1 
wenn das bein ganz raungezogen ist der cos(theta) * n = 0 weil projeziert lönge 0!

wie komme ich jetzt an den winkel:
Full extended etha = 1
der acos(1) = 0,
man dreht um 0 grad
ansonsten um 90 grad
 */



/// @brief will clamp the etha value and return the angle for the hip, positive
/// @param etha etha value
/// @return angle for the hip pitch from etha as dot product 
float TwoBone::angleFromEtha(float etha){
    //mit dem arcus cosinus einen cosinus wert wieder zu winkel bauen, um dann zu rotieren
    if(etha < 0.0f){
        etha = 0.0f;
    }
    if(etha > 1.0f){
        etha = 1.0f;
    }
    
    float lambda = std::acosf(etha); //testing. Richtig! --> 1, ganz paralell, 0 grad.
    return lambda;
}


/// @brief returns the etha (angle) value for a distance
/// @param distance distance from 0,0,0 to target
/// @return etha value, dot product
float TwoBone::createEthaFromDistance(float distance){
    if(totalBoneLengthCopy == 0){
        return 0; //fully extended error
    }
    //ganz ausgestreckt: skalarprodukt = 1
    //ansonsten eingekickt. Ganz angezogen: skalarprodukt 0: 90 grad
    float etha = (distance / totalBoneLengthCopy);
    return etha;
}




/// @brief creates the hip angle and signs it autmatically
/// @param angle angle for the hip
/// @return angle signed, ready for matrix pitch apply
float TwoBone::createHipAngle(float angle){
    return angle *-1;
}
/// @brief creates the knee angle and multiplies in automatically
/// @param angle angle for the knee 
/// @return signed and multiplied angle for knee, ready for matrix pitch apply
float TwoBone::createKneeAngle(float angle){
    return angle * 2;
}




/**
 * 
 * 
 * Allgemiennen cosinus satz nutzen um winkel der 
 * 2 knochen zu bestimmen
 * 
 * 
 */


/// @brief will calculate the angles based on the bone lengths and override first and second output
/// for the root joint and second joint pitch angle
/// @param distanceTotarget 
/// @param firstOuput for root joint
/// @param secondOutput for second joint
/// @param forwardIK 
/// @param middle second joint matrix (translation is needed for bone length)
/// @param end third joint matrix (translation is needed for bone length)
void TwoBone::createEthaPitchAnglesFor(
    float distanceTotarget, 
    float &firstOuput, 
    float &secondOutput, 
    bool forwardIK,
    MMatrix &middle,
    MMatrix &end
){
    
    float _c = std::abs(distanceTotarget);
    float _b = std::abs(middle.getTranslation().Size());
    float _a = std::abs(end.getTranslation().Size());

    float a2 = _a * _a;
    float b2 = _b * _b;
    float c2 = _c * _c;

    /*
    float alpha = std::acosf(((b2 + c2 - a2) / (2 * _b * _c)));
    float beta = std::acosf(((a2 + c2 - b2) / (2 * _a * _c)));
    float gamma = std::acosf(((a2 + b2 - c2) / (2 * _a * _b)));
    */
   

    float alpha = std::acosf(FMath::Clamp((b2 + c2 - a2) / (2 * _b * _c), -1.0f, 1.0f));
    float beta = std::acosf(FMath::Clamp((a2 + c2 - b2) / (2 * _a * _c), -1.0f, 1.0f));
    float gamma = std::acosf(FMath::Clamp((a2 + b2 - c2) / (2 * _a * _b), -1.0f, 1.0f));



    firstOuput = -1 * alpha;
    //secondOutput = alpha * 2; //*-2 um den winkel einfach zu flippen bei gleichgrossen knochen
    
    //debug disable
    gamma = MMatrix::degToRadian(180.0f - std::abs(MMatrix::radToDegree(gamma)));
    secondOutput = gamma;

    /*FString debugAngleString = FString::Printf(
        TEXT("AngleDebug alpha HIP %.2f, gamma KNEE %.2f"),
        MMatrix::radToDegree(alpha),
        MMatrix::radToDegree(gamma)
    );*/


}

/**
 * 
 * BUILDING BONES
 * 
 */

/// @brief fills all matrix data into the array and the foot tip into the out vector
/// @param dataout output for matricies
/// @param outVector output for foot tip vektor
void TwoBone::getMatricies(std::vector<MMatrix*> &dataout, FVector &outVector){ 
    
    dataout.push_back(&hip);
    dataout.push_back(&knee);
    dataout.push_back(&foot);
    outVector = toFootTip; //assign to out
}





void TwoBone::build(
    UWorld *world, 
    MMatrix &offsetAndRotation, 
    MMatrix &endEffector, 
    FColor color, 
    float displayTime
){

    std::vector<MMatrix *> matrizen;
    FVector endVec;
    getMatricies(matrizen, endVec);

    //new testing
    std::vector<AActor *> limbs;
    limbs.push_back(hipLimbPointer);
    limbs.push_back(kneeLimbPointer);
    limbs.push_back(footLimbPointer);


    MMatrix newStart = buildWithOutput(
        world,
        offsetAndRotation,
        color,
        displayTime,
        matrizen,
        limbs,
        true //forward
    );


    //copyCurrentMatricies();

    FVector translation = newStart.getTranslation();
    endEffector.setTranslation(translation);
}



/// @brief will rotate the complete bone (chain) towards a target location with a weight
void TwoBone::rotateEndToTarget(FVector &vec, FVector &weight){
    rotateEndToTarget(vec, weight, hip, knee, foot, true);
}


/// @brief is tested for forward and backward kinematics!
/// @param vec local target when x is forward
/// @param weight weight direction on middle joint limb
/// @param start start limb (foot or hip) (recommenden to be the empty starting einheits matrix)
/// @param middle middle limb joint -> ankle / ell / knee limb
/// @param end end limb foot / hand or hip / opposite of start!
void TwoBone::rotateEndToTarget(
    FVector &vec,
    FVector &weight,
    MMatrix &start,
    MMatrix &middle,
    MMatrix &end,
    bool debugForwardIk
)
{
    //TODO: add support for using the rotation which is already applied? 
    start.resetRotation();
    middle.resetRotation();
    end.resetRotation();


    /**
     * Top view yaw rotation of weight
     * 
     * gewicht ziegt ja irgendwo in zy pane und dann wird die bein achse (um z) gespinnt.
     */
    if(std::abs(weight.Y) >= 0.1f){ //gegen epsilon prüfen.
        float yawAngleWeight = yawAngleTo(weight); // rollAngleTo(weight);
        start.yawRadAdd(yawAngleWeight); //yaw drehen weil fuss erstmal nach unten zeigt, rotiert um eigene achse
    }

    //verbessert den arm bug aber behebt ihn nicht ganz
    if(isArmBone()){ //DEBUG
        
        //new global start rotator testing
        MMatrix rotator = MMatrix::createRotatorFrom(vec, FVector2D(1, 0), FVector2D(0, -1));
        start *= rotator;



    }

    
    //normalize the target location if exceeding the bone lenght
    float distance = vec.Size();
    if (distance > totalBoneLengthCopy)
    {
        vec = vec.GetSafeNormal();
        vec *= totalBoneLengthCopy;
        distance = totalBoneLengthCopy; //clamp
    }

    // --- KNICK BASIS ---
    
    float hipAngle = 0.0f;
    float kneeAngle = 0.0f;

    createEthaPitchAnglesFor(
        distance, 
        hipAngle, 
        kneeAngle, 
        debugForwardIk,
        middle,
        end
    );


    //WEIGHT KNICK RICHTUNG
    //anhand des wights dann knick winkel flippen
    if(flipAngleForBoneNeeded(vec, weight, hipAngle)){ 
        hipAngle *= -1.0f;
        kneeAngle *= -1.0f;
    }

    //ETHA & WEIGHT ---> funktioniert auch wie erwartet 
    start.pitchRadAdd(hipAngle);   //hip nach vorne
    middle.pitchRadAdd(kneeAngle); //knee to foot
    end.pitchRadAdd(hipAngle);

    



    // --- GLOBAL PITCH TO TARGET ROTATION ---
    
    //die matrix zeigt zunächst immer nach unten, so ist der limb im konstruktor definiert
    //und so muss auch die rotation gefunden werden, egal ob vorwärts
    //oder rückwärts! Der winkel ist beide male korrekt!
    
    //test
    if(!isArmBone()){
        float pitchAngle = pitchAngleToInitialLookDirOfBone(vec); 
        float globalSideAdd = createHipAngle(pitchAngle);
        start.pitchRadAdd(globalSideAdd);
    
    }
    //old
    /*float pitchAngle = pitchAngleToInitialLookDirOfBone(vec); 
    float globalSideAdd = createHipAngle(pitchAngle);
    start.pitchRadAdd(globalSideAdd);*/
    

    


}

bool TwoBone::flipAngleForBoneNeeded(FVector &target, FVector &weight, float hipAngle){

    bool flipByWeight = (weight.X < 0.0f || weight.Z < 0.0f); //wenn gewicht negativ
    bool negativeHip = hipAngle < 0.0f; //angle zeigt grade nach vorne
    if(flipByWeight && negativeHip){
        return true;
    }

    if(isArmBone()){
        bool negativeTarget = target.X < 0.0f;
        if(negativeTarget){
            return true;
        }
    }
    
    return false;
}

/// @brief fllag for global yaw rotation, is blocked for leg movement because of. Bugs.
/// @return is meant to be an arm or not
bool TwoBone::isArmBone(){
    return isArmFlag;
}


bool TwoBone::pitchTooLow(float globalPitch){
    if(std::abs(MMatrix::radToDegree(globalPitch)) < 45.0f){
        return true;
    }
    return false;
}

/// @brief calculates the yaw angle (from top perspektive) to a local target
/// making the leg for example turn as a whole to the left or right
/// like a top view 2d rotation
/// (apply only on first joint if rotation along axis wanted)
/// @param localTarget 
/// @return signed rotation on xy pane from top
float TwoBone::yawAngleTo(FVector &localTarget){
    
    

    //calculate angle
    FVector2D forward2d(1.0f, 0);
    
    FVector2D xy(localTarget.X, localTarget.Y);
    xy = xy.GetSafeNormal(); //nur 2d normalisieren weil sonst fehler auftreten, nicht 3D!

    //RECHUNG:
    //cos(theta) = a dot b if a and b are normalized!
    /*
        cos(theta) = a * b | cos^-1
        theta = acos((a*b) / (|a| * |b|))

        bzw weil a und b length 1 sind:
        theta = acos((a*b) / 1)
    */

    float dot = forward2d.X * xy.X; //+ forward2d.Y * xy.Y;

    float xyTopViewAngle = std::acosf(dot);
    

    xyTopViewAngle *= flipRotation(forward2d.X, forward2d.Y, xy.X, xy.Y);

    return xyTopViewAngle;
}

/// @brief calculates the angle on xz pane to a local target (making the)
/// pitch rotation for example for the hip
/// @param localTarget target when x is forward
/// @return abs angle on xz pane, NOT Signed
float TwoBone::pitchAngleTo(FVector &localTarget){
    
    FVector2D forward2d(1, 0);
    forward2d = forward2d.GetSafeNormal();

    FVector2D xz(localTarget.X, localTarget.Z);
    xz = xz.GetSafeNormal(); //nur 2d normalisieren weil sonst fehler auftreten, nicht 3D!

    float xzSideViewAngle = std::acosf(FVector2D::DotProduct(forward2d, xz));
    //KEINE flip rotation hier, ist ein einzelfall
    return xzSideViewAngle;
}



/// @brief calculates the angle on yz pane to a local target (making the)
/// roll rotation for example for the hip
/// (apply only on first joint if rotation along axis wanted)
/// @param localTarget 
/// @return roll angle between forward and target, signed!
float TwoBone::rollAngleTo(FVector &localTarget){
    //FVector forward(1, 0, 0); //sollte member var werden ggf
    FVector2D forward2d(0, 1); //z ist up, -z ist initial 

    FVector2D yz(localTarget.Y, localTarget.Z);
    yz = yz.GetSafeNormal(); //nur 2d normalisieren weil sonst fehler auftreten, nicht 3D!

    float yzSideViewAngle = std::acosf(FVector2D::DotProduct(forward2d, yz));

    //testing needed
    yzSideViewAngle *= flipRotation(forward2d.X, forward2d.Y, yz.X, yz.Y);


    return yzSideViewAngle;
}


/// @brief will tell if a angle between vectors is negative or not (Determines the sign of angle)
/// @param aX x comp of vector a
/// @param aY y comp of vector a
/// @param oX x comp of vector b
/// @param oY y comp of vector b
/// @return +1 for counter clock wise rotation or -1. Multiply your angle with the returned
/// value to make your angle signed 
float TwoBone::flipRotation(float aX, float aY, float oX, float oY){
    //kreuzproduk 2D
    //A:=(a,b,0) B:=(c,d,0)
    //normal := A cross B
    //normal.Z = ad - bc 

    // Bestimmen der Drehrichtung durch das Kreuzprodukt
    float crossProduct_z = aX * oY - aY * oX;

    // Wenn das Kreuzprodukt negativ ist, ist der Winkel im Uhrzeigersinn,
    // also negieren wir den Winkel.
    if (crossProduct_z < 0.0f) {
        return -1.0f;
    }
    return 1.0f;
}

/// @brief resets the rotationof hip knee and foot to original position
void TwoBone::resetAllRotations(){
    //float rad = MMatrix::degToRadian(0);

    hip.resetRotation();
    knee.resetRotation();
    foot.resetRotation();

}



float TwoBone::pitchAngleToInitialLookDirOfBone(FVector &localTarget){
    FVector2D forward2d(0.0f, -1.0f); //0, -1
    forward2d = forward2d.GetSafeNormal();

    FVector2D xz(localTarget.X, localTarget.Z);
    xz = xz.GetSafeNormal(); //nur 2d normalisieren weil sonst fehler auftreten, nicht 3D!

    
    float xzSideViewAngle = std::acosf(FVector2D::DotProduct(forward2d, xz));
    xzSideViewAngle *= flipRotation(forward2d.X, forward2d.Y, xz.X, xz.Y);

    return xzSideViewAngle;
}


float TwoBone::pitchAngleToInitiaToUpDirOfBone(FVector &localTarget){
    FVector2D forward2d(0, 1); //0, -1
    forward2d = forward2d.GetSafeNormal();

    FVector2D xz(localTarget.X, localTarget.Z);
    xz = xz.GetSafeNormal(); //nur 2d normalisieren weil sonst fehler auftreten, nicht 3D!

    
    float xzSideViewAngle = std::acosf(FVector2D::DotProduct(forward2d, xz));
    xzSideViewAngle *= flipRotation(forward2d.X, forward2d.Y, xz.X, xz.Y);

    return xzSideViewAngle;
}









/// @brief DESIGNED FOR FROM FOOT ADJUSTING HIP!
/// @param world 
/// @param vec target relative to foot
/// @param weight weight dir of angle
/// @param offsetAndRotation END EFFECTOR WORLD (Hand or foot to keep)
/// expects correct starting matrix where start limb is located originally (hip or grounded foot)
/// @param translationOfactor START EFFECTOR ADJUST overrides translation of actor based on where hip was moved
/// @param color 
/// @param displayTime 
void TwoBone::rotateStartToTargetAndBuild( //works as expected
    UWorld*world, 
    FVector &vec, 
    FVector &weight, 
    MMatrix &offsetAndRotationOfFoot, 
    MMatrix &translationOfactor, //overrides translation of actor based on where hip was moved, start martix
    FColor color,
    float displayTime
){
    //weight *= -1;
    weight.X *= -1; //sonst falschrum, umdrehen weil rechnung umgedreht! nicht ändern, stimmt so, X is Forward!

    std::vector<MMatrix *> matrizen;

    
    MMatrix empty; //NEW START (Hip but backwards, ersatz.)
    rotateEndToTarget(vec, weight, empty, foot, knee, false);
    

    matrizen.push_back(&empty); //damit first limb gezeichnet wird von foot to knee usw. Matrix multiplikation
    matrizen.push_back(&foot); 
    matrizen.push_back(&knee);
    //matrizen.push_back(&hip); //damit end limb geziechnet wird


    //reverse bones
    std::vector<AActor *> bonesAttached;
    bonesAttached.push_back(hipLimbPointer);
    bonesAttached.push_back(kneeLimbPointer);
    bonesAttached.push_back(footLimbPointer);

    

    
    
    MMatrix newStart = buildWithOutput(
        world,
        offsetAndRotationOfFoot,
        color,
        displayTime,
        matrizen,
        bonesAttached, //new testing needed
        false
    );
    

    //will probably make matrix shoot in the air.
    FVector newLocation = newStart.getTranslation();

    //Override translation of actor (hip)
    translationOfactor.setTranslation(newLocation);

    //backup
    //copyCurrentMatricies();
}








/// @brief interpolates the matricies and returns the final output
/// @param world 
/// @param offsetAndRotation 
/// @param color 
/// @param displayTime 
/// @param matrizen 
/// @return 
MMatrix TwoBone::buildWithOutput(
    UWorld *world,
    MMatrix &offsetAndRotation,
    FColor color, 
    float displayTime,
    std::vector<MMatrix*> &matrizen //must not be empty
){

    

    FVector endVec = toFootTip;
    // getMatricies(matrizen, endVec);
    if(matrizen.size() < 2){ // <2 weil mindestens ein knochen muss vorhanden sein
        return offsetAndRotation;
    }

    std::vector<FVector> resultDraw;

    MMatrix result = offsetAndRotation;
    
    //über matrizen laufen um die vektoren zu berechnen für die zeichnung
    //beide sollen nicht mit gezeichnet werden
    for (int i = 0; i < matrizen.size(); i++)
    {
        result *= *matrizen[i]; //durch das multiplizieren der matrizen wandert man sie entlang

        resultDraw.push_back(result.getTranslation()); //translation will be now in result space always
    }

    //final vector
    FVector outputVec = result * endVec;
    resultDraw.push_back(outputVec);



    //draw
    if (world != nullptr && DEBUG_DRAW)
    {
        for (int i = 1; i < resultDraw.size(); i++)
        {
            DebugHelper::showLineBetween(world, resultDraw[i - 1], resultDraw[i], color, displayTime);
        }
    }

    //returns the final matrix
    return result;
}





/**
 * NEW TARGET MOVEMENT AND UPDATE FOOT MATRIX
*/

/// @brief rotate end to a target and apply new foot pos if needed!
/// @param world 
/// @param target local target of foot if x axis (1,0,0) is forward
/// @param weight weight direction to apply 
/// @param offsetAndRotation calculated transform of actor 
///                         (M = location * rotation <--read direction--)
/// @param endEffectorWorldToUpdate translation of foot of actor which will be modified! ONLY TRANSLATION!
/// @param color color to draw in
/// @param displayTime displaytime to draw
void TwoBone::rotateEndToTargetAndBuild(
    UWorld*world, 
    FVector &target, 
    FVector &weight, 
    MMatrix &offsetAndRotation, 
    MMatrix &endEffectorWorldToUpdate,
    FColor color,
    float displayTime
){
    rotateEndToTarget(target, weight); //DeltaTime);

    std::vector<MMatrix *> matrizen;
    FVector footTip;
    getMatricies(matrizen, footTip);


    //new testing
    std::vector<AActor *> limbs;
    limbs.push_back(hipLimbPointer);
    limbs.push_back(kneeLimbPointer);
    limbs.push_back(footLimbPointer);


    MMatrix newStart = buildWithOutput(
        world,
        offsetAndRotation,
        color,
        displayTime,
        matrizen,
        limbs,
        true
    );

    //will probably make matrix shoot in the air.
    FVector newLocation = newStart.getTranslation();

    //Override translation of actor (FOOT!)
    endEffectorWorldToUpdate.setTranslation(newLocation);

    //copyCurrentMatricies();
}





/// @brief PASSED LIMB REFERENCE MUST NOT EVER GET INVALIDATED!
/// @param actorToAttach 
void TwoBone::attachFirtsLimb(AActor &actorToAttach){
    if(!actorIsAlreadAttached(actorToAttach)){
        hipLimbPointer = &actorToAttach;
        disableLimbCollision(*hipLimbPointer);
    }
}

void TwoBone::attachSecondLimb(AActor &actorToAttach){
    if(!actorIsAlreadAttached(actorToAttach)){
        kneeLimbPointer = &actorToAttach;
        disableLimbCollision(*kneeLimbPointer);
    }
}

void TwoBone::attachThirdLimb(AActor &actorToAttach){
    if(!actorIsAlreadAttached(actorToAttach)){
        footLimbPointer = &actorToAttach;
        disableLimbCollision(*footLimbPointer);
    }
}

bool TwoBone::actorIsAlreadAttached(AActor &actor){
    return &actor == footLimbPointer ||
           &actor == kneeLimbPointer ||
           &actor == hipLimbPointer;
}


/// @brief disbales physics collision for an actor
/// @param actor actor physics collision to disable, NOT raycasting
void TwoBone::disableLimbCollision(AActor& actor){
    if (UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(actor.GetRootComponent())) {
        RootComp->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
    }
}



void TwoBone::updateLimb(AActor *limb, MMatrix &matrixRotator, FVector location){
    //rotator erstellen und auf actor anwenden
    if(limb != nullptr){
        limb->SetActorLocation(location);
        limb->SetActorRotation(matrixRotator.extractRotator());
    }
}







/**
 * 
 * 
 * 
 * ----- new section with limb update ------
 * 
 * 
 */

/// @brief interpolates the matricies and returns the final output
/// @param world 
/// @param offsetAndRotation 
/// @param color 
/// @param displayTime 
/// @param matrizen 
/// @return 
MMatrix TwoBone::buildWithOutput(
    UWorld *world,
    MMatrix &offsetAndRotation,
    FColor color, 
    float displayTime,
    std::vector<MMatrix*> &matrizen, //must not be empty
    std::vector<AActor*> &attachedBones,
    bool forward
){


    FVector endVec = toFootTip;    
    if(matrizen.size() < 2){ // <2 weil mindestens ein knochen muss vorhanden sein
        return offsetAndRotation;
    }

    std::vector<MMatrix> resultMatricies;
    std::vector<FVector> resultDraw;
    MMatrix result = offsetAndRotation;

    //über matrizen laufen um die vektoren zu berechnen für die zeichnung
    //beide sollen nicht mit gezeichnet werden
    for (int i = 0; i < matrizen.size(); i++)
    {
        result *= *matrizen[i]; //durch das multiplizieren der matrizen wandert man sie entlang
        if(forward){
            resultDraw.push_back(result.getTranslation()); //translation will be now in result space always
            resultMatricies.push_back(result); //debug blocked
        }
    }

    //final vector (proper if forward pass)
    FVector outputVec;
    if (forward){
        outputVec = result * endVec;
        resultDraw.push_back(outputVec);
    }

    //WENN RÜCKWÄRTS: NOCHMAL!
    //inversen holen für umkehrpass
    //kette neu zeichnen, gedreht.
    if (!forward)
    {
        resultDraw.clear();
        std::vector<MMatrix> inverted;
        for (int i = matrizen.size() - 1; i >= 0; i--){
            MMatrix currentInvert = matrizen[i]->jordanInverse(); // createInverse();
            inverted.push_back(currentInvert);
        }

        MMatrix start = result;
        resultDraw.push_back(start.getTranslation());
        for (int i = 0; i < inverted.size(); i++){
            
            start *= inverted[i];
            resultDraw.push_back(start.getTranslation());
            resultMatricies.push_back(start);
        }
        outputVec = start * endVec;
        resultDraw.push_back(outputVec);
    }





    //draw
    if (world != nullptr && DEBUG_DRAW)
    {
        for (int i = 1; i < resultDraw.size(); i++){
            if(isArmBone()){
                if(i != resultDraw.size() - 1){
                    DebugHelper::showLineBetween(world, resultDraw[i - 1], resultDraw[i], color, displayTime);
                }
            }else{
                DebugHelper::showLineBetween(world, resultDraw[i - 1], resultDraw[i], color, displayTime);
            }
            
        }
    }


    

    //update bones based on matricies build
    for(int i = 0; i < resultMatricies.size(); i++){
        MMatrix &current = resultMatricies.at(i);
        FVector location = current.getTranslation();

        if (i < attachedBones.size()){
            AActor *currentBone = attachedBones[i];
            updateLimb(currentBone, current, location);
        }
    }
    return result;

    
}




//copy data for outside use
void TwoBone::copyCurrentMatricies(){
    hipCopy = hip;
	kneeCopy = knee;
	footCopy = foot;
}

FVector TwoBone::endLimbWorldLocation(){
    return footCopy.getTranslation();
}