// Fill out your copyright notice in the Description page of Project Settings.

#include "SSCCameraComponent.h"

#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/SplineComponent.h"

#include "SSCBlueprintFunctionLibrary.h"
#include "SideScrollerFollowComponent.h"
#include "SSCActorOverlapComponent.h"
#include "SSCGameMode.h"


// Sets default values for this component's properties
USSCCameraComponent::USSCCameraComponent()
{
// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
// off to improve performance if you don't need them.
PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void USSCCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	// Update protectedVariables to Default Camera Settings
	UpdateCameraParameters(DefaultCameraParameters);

	// Set Camera Angle
	GetOwner()->SetActorRotation(DefaultCameraAngle);

	UWorld* TheWorld = GetWorld();
	if (TheWorld != nullptr)
	{
		AGameModeBase* GameMode = UGameplayStatics::GetGameMode(TheWorld);
		ASSCGameMode* SSCGameMode = Cast<ASSCGameMode>(GameMode);
		if (SSCGameMode != nullptr)
		{
			SSCGameMode->UpdateCameraDelegate.AddDynamic(this, &USSCCameraComponent::UpdateCameraParameters);
		}

	}


	// Get all actors to follow
	for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		TArray<USideScrollerFollowComponent*> SideScrollerFollowComponents;
		ActorItr->GetComponents(SideScrollerFollowComponents);

		if (SideScrollerFollowComponents.Num() > 0)
		{
			ActorsToFollow.Add(*ActorItr);
			UE_LOG(SSCLog, Log, TEXT("Following Actor: %s"), *ActorItr->GetName());
			SetCameraLocation(GetActorsLocation());
		}

		TArray<USSCOverlapComponent*> SSCOverlapComponents;
		ActorItr->GetComponents(SSCOverlapComponents);

		if (SSCOverlapComponents.Num() > 0)
		{
			auto SSCOverlapComponent = Cast<USSCOverlapComponent>(SSCOverlapComponents[0]);
			//SSCOverlapComponent->OnOverlapWithOverlapComponent.AddDynamic(this, &USSCCameraComponent::UpdateCameraParameters);
		}
	}

	// Check for manual camera rotation settings
	if (bManualCameraRotation)
	{
		UE_LOG(SSCLog, Log, TEXT("ManualCameraRotation activated"));
		if (RotateCameraXAxisMappingName != TEXT("None"))
		{
			if (GetOwner() != nullptr)
			{
				PlayerController = Cast<APlayerController>(GetWorld()->GetFirstPlayerController());

				if (PlayerController != nullptr) //&& PlayerController->InputEnabled == false)
				{
					GetOwner()->EnableInput(PlayerController);

					GetOwner()->InputComponent->BindAxis(RotateCameraXAxisMappingName); // <- hier nochmal schauen, ob das so okay oder der beste Weg ist
					GetOwner()->InputComponent->BindAxis(RotateCameraYAxisMappingName); // <- hier nochmal schauen, ob das so okay oder der beste Weg ist
				}
			}
		}
		else
		{
			UE_LOG(SSCLog, Warning, TEXT("ManualCameraRotation is activated, but no RotateCameraXAxisMapping is set"));
		}
	}

}


// Called every frame
void USSCCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SetCameraLocation(GetActorsLocation());

	if (bManualCameraRotation && PlayerController != nullptr)
	{
		ManuallyRotateCamera(DeltaTime);
	}
	else
	{
		if (PlayerController == nullptr)
		{
			UE_LOG(SSCLog, Warning, TEXT("MaualCameraRotation is set to true, but PlayerController is null"))
		}
	}
}

void USSCCameraComponent::SetCameraLocation(FVector ActorsLocation)
{
	/*if (ActorsLocation.IsZero())
	{
	UE_LOG(SSCLog, Log, TEXT("No Actor To Follow and DefaultCameraType set to Follow"));
	return;
	}*/

	FVector NewLocation;

	switch (ProtectedCameraParametersInstance.SSCType)
	{
		
		case ESSCTypes::Static:
			// Set camera to DefaultTargetLocation and apply z-axis camera-offset
			NewLocation = ProtectedCameraParametersInstance.TargetLocation + GetOwner()->GetActorUpVector() * DefaultCameraOffsetZ;

			if (ProtectedCameraParametersInstance.CameraSplinePath)
			{
				UE_LOG(SSCLog, Warning, TEXT("Camera is set to static, but CameraSplinePath is activated"));
			}
			break;


		case ESSCTypes::Follow:
			// Camera on Spline not tested at all
			if (ProtectedCameraParametersInstance.bCameraSplinePath)
			{
				if (ProtectedCameraParametersInstance.CameraSplinePath != nullptr)
				{
					TArray<USplineComponent*> SplineComponents;
					ProtectedCameraParametersInstance.CameraSplinePath->GetComponents<USplineComponent>(SplineComponents);
					switch (SplineComponents.Num())
					{
					case 0:
						UE_LOG(SSCLog, Error, TEXT("CameraSplinePath is activated, but CameraSplinePath Actor has no Spline Component"));
						break;
					case 1:
						NewLocation = SplineComponents[0]->FindLocationClosestToWorldLocation(ActorsLocation, ESplineCoordinateSpace::World);
						break;
					default:
						UE_LOG(SSCLog, Error, TEXT("CameraSplinePath Actor has multiple Spline Components, dont know wich to follow"));
						break;
					}
				}
				else
				{
					UE_LOG(SSCLog, Error, TEXT("Camera is set to Spline, but no CameraSplinePath is linked"));
				}
			}
			else
			{
				// Set camera distance in relation to rotation
				NewLocation = ActorsLocation + GetOwner()->GetActorForwardVector() * ProtectedCameraParametersInstance.Armlength + GetOwner()->GetActorUpVector() * DefaultCameraOffsetZ;
			}

			

			break;


		case ESSCTypes::Cylindrical:
			// Cylindrical movement along spline - not tested at all
			if (ProtectedCameraParametersInstance.bCameraSplinePath)
			{
				if (ProtectedCameraParametersInstance.CameraSplinePath != nullptr)
				{
					TArray<USplineComponent*> SplineComponents;
					ProtectedCameraParametersInstance.CameraSplinePath->GetComponents<USplineComponent>(SplineComponents);
					switch (SplineComponents.Num())
					{
						case 0:
							UE_LOG(SSCLog, Error, TEXT("CameraSplinePath is activated, but CameraSplinePath Actor has no Spline Component"));
							break;
						case 1:
							NewLocation = SplineComponents[0]->FindLocationClosestToWorldLocation(ActorsLocation, ESplineCoordinateSpace::World);
							UE_LOG(SSCLog, Warning, TEXT("CamerSplinePath and cylindrical camery type is active and will be executed, but it doesnt make that much sense"))
							break;
						default:
							UE_LOG(SSCLog, Error, TEXT("CameraSplinePath Actor has multiple Spline Components, dont know wich to follow"));
							break;
					}
				}
				else
				{
					UE_LOG(SSCLog, Error, TEXT("Camera is set to Spline, but no CameraSplinePath is linked"));
				}
			}
			else
			{
				// Set camera location always facing the center with a fixed distance (armlength) to the center of all followed actors...
				NewLocation = ActorsLocation - ProtectedCameraParametersInstance.CenterOfCylindricalMovement;
				NewLocation = NewLocation.GetSafeNormal();
				NewLocation = NewLocation * ProtectedCameraParametersInstance.Armlength * -1 + ActorsLocation;
			}
			// ... with a rotation suitable to all followed actors
			GetOwner()->SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetOwner()->GetActorLocation(), ActorsLocation));
			break;
		

		// Camera on Spline over time not tested at all
		case ESSCTypes::SplineOverTime:
			if (ProtectedCameraParametersInstance.CameraSplinePath == nullptr)
			{
				break;
			}
			else
			{
				TArray<USplineComponent*> SplineComponents;
				ProtectedCameraParametersInstance.CameraSplinePath->GetComponents<USplineComponent>(SplineComponents);
				if (SplineComponents.Num() == 1) {
					NewLocation = SplineComponents[0]->GetLocationAtDistanceAlongSpline(Splinepoint, ESplineCoordinateSpace::World);
					GetOwner()->SetActorRotation(SplineComponents[0]->GetRotationAtDistanceAlongSpline(Splinepoint, ESplineCoordinateSpace::World) + FRotator(0, -90, 0)); // nicht ganz sicher, ob das korrekt funktioniert
					if (SplineOverTimeTimeElapsed < ProtectedCameraParametersInstance.SplineOverTimeTime)
					{
						SplineOverTimeTimeElapsed += GetWorld()->GetDeltaSeconds();
						Splinepoint = Splinepoint + SplineLength * GetWorld()->GetDeltaSeconds() / ProtectedCameraParametersInstance.SplineOverTimeTime;
					}
				}
			}
			break;
	};	

	// Calculate camera location with InterpolationSpeed
	if (bInterpolationSpeed && ProtectedCameraParametersInstance.SSCType != ESSCTypes::Static)
	{
		NewLocation = FMath::VInterpConstantTo(GetOwner()->GetActorLocation(), NewLocation, GetWorld()->GetDeltaSeconds(), InterpolationSpeed);
	}


	GetOwner()->SetActorLocation(NewLocation);

}

// Update Camera Parameters
void USSCCameraComponent::UpdateCameraParameters(FUpdateCameraParametersStruct newCameraParameters)
{
	ProtectedCameraParametersInstance = newCameraParameters;

	if (newCameraParameters.SSCType == ESSCTypes::SplineOverTime)
	{
		SplineOverTimeTimeElapsed = 0;
		
		if (ProtectedCameraParametersInstance.CameraSplinePath != nullptr)
		{
			TArray<USplineComponent*> SplineComponents;
			ProtectedCameraParametersInstance.CameraSplinePath->GetComponents<USplineComponent>(SplineComponents);
			switch (SplineComponents.Num())
			{
			case 0:
				UE_LOG(SSCLog, Error, TEXT("CameraSplinePath is activated, but CameraSplinePath Actor has no Spline Component"));
				break;
			case 1:
				SplineLength = SplineComponents[0]->GetSplineLength();
				break;
			default:
				UE_LOG(SSCLog, Error, TEXT("CameraSplinePath Actor has multiple Spline Components, dont know wich to follow"));
				break;
			}
		}
		else
		{
			UE_LOG(SSCLog, Error, TEXT("Camera is set to SplineOverTime, but no CameraSplinePath is linked"));
		}
	}
}

// Returns center of all followed Actors
FVector USSCCameraComponent::GetActorsLocation()
{
	if (ActorsToFollow.Num() == 0) {
		return{};
	}

	FVector ActorsLocation;

	for (AActor* Actors : ActorsToFollow)
	{
		ActorsLocation += Actors->GetActorLocation();
		//UE_LOG(SSCLog, Log, TEXT("Following Actor %s"), *Actors->GetName());

	}

	ActorsLocation = ActorsLocation / ActorsToFollow.Num();

	ActorsToFollowLocation = ActorsLocation;

	// If manual camera backwards movement is dependent on movement of followed actors, call AreActorsMoving function
	// Dunno if this is here okay or better called from tick (with skippable frames?)
	if (bManualCameraBackwardsRotationWhenMoving == true)
	{
		AreActorsMoving(ActorsToFollowLocation);
	}

	return ActorsToFollowLocation;
}

// returns if one of the actors to follow is moving
bool USSCCameraComponent::AreActorsMoving(FVector ActorsToFollowLocation)
{
	if (ActorsToFollowLocationLastFrame == FVector(0, 0, 0)) //auch mal schauen, obs da nichts besseres gibt
	{
		ActorsToFollowLocationLastFrame = ActorsToFollowLocation;
		bAreActorsToFollowMoving = false;
	}
	else if (ActorsToFollowLocation == ActorsToFollowLocationLastFrame)
	{
		bAreActorsToFollowMoving = false;
	}
	else
	{
		bAreActorsToFollowMoving = true;
	}

	ActorsToFollowLocationLastFrame = ActorsToFollowLocation;
	
	return bAreActorsToFollowMoving;
}

// Manual Camera Movement
void USSCCameraComponent::ManuallyRotateCamera(float DeltaTime)
{

	// manual camera rotation
	if (GetOwner()->GetInputAxisValue(RotateCameraXAxisMappingName) != 0 || GetOwner()->GetInputAxisValue(RotateCameraYAxisMappingName) != 0)
	{
		if (GetOwner()->GetInputAxisValue(RotateCameraXAxisMappingName) > 0 && (ManualCameraXValue + GetOwner()->GetInputAxisValue(RotateCameraXAxisMappingName) * DeltaTime * ManualCameraRotationSpeed) <= ManualCameraMaxYawValue)
		{
			ManualCameraXValue = ManualCameraXValue + GetOwner()->GetInputAxisValue(RotateCameraXAxisMappingName) * DeltaTime * ManualCameraRotationSpeed;
			GetOwner()->AddActorLocalRotation(FRotator((GetOwner()->GetInputAxisValue(RotateCameraXAxisMappingName)) * DeltaTime * ManualCameraRotationSpeed, 0, 0));
		}
		else if (GetOwner()->GetInputAxisValue(RotateCameraXAxisMappingName) < 0 && (ManualCameraXValue + GetOwner()->GetInputAxisValue(RotateCameraXAxisMappingName) * DeltaTime * ManualCameraRotationSpeed) >= ManualCameraMaxYawValue * -1)
		{
			ManualCameraXValue = ManualCameraXValue + GetOwner()->GetInputAxisValue(RotateCameraXAxisMappingName) * DeltaTime * ManualCameraRotationSpeed;
			GetOwner()->AddActorLocalRotation(FRotator((GetOwner()->GetInputAxisValue(RotateCameraXAxisMappingName)) * DeltaTime * ManualCameraRotationSpeed, 0, 0));
		}

		if (GetOwner()->GetInputAxisValue(RotateCameraYAxisMappingName) > 0 && (ManualCameraYValue + GetOwner()->GetInputAxisValue(RotateCameraYAxisMappingName) * DeltaTime * ManualCameraRotationSpeed) <= ManualCameraMaxPitchValue)
		{
			ManualCameraYValue = ManualCameraYValue + GetOwner()->GetInputAxisValue(RotateCameraYAxisMappingName) * DeltaTime * ManualCameraRotationSpeed;
			GetOwner()->AddActorLocalRotation(FRotator(0, (GetOwner()->GetInputAxisValue(RotateCameraYAxisMappingName)) * DeltaTime * ManualCameraRotationSpeed, 0));
		}
		else if (GetOwner()->GetInputAxisValue(RotateCameraYAxisMappingName) < 0 && (ManualCameraYValue + GetOwner()->GetInputAxisValue(RotateCameraYAxisMappingName) * DeltaTime * ManualCameraRotationSpeed) >= ManualCameraMaxPitchValue * -1)
		{
			ManualCameraYValue = ManualCameraYValue + GetOwner()->GetInputAxisValue(RotateCameraYAxisMappingName) * DeltaTime * ManualCameraRotationSpeed;
			GetOwner()->AddActorLocalRotation(FRotator(0, (GetOwner()->GetInputAxisValue(RotateCameraYAxisMappingName)) * DeltaTime * ManualCameraRotationSpeed, 0));
		}
		
		GetOwner()->SetActorRotation(FRotator(GetOwner()->GetActorRotation().Pitch, GetOwner()->GetActorRotation().Yaw, 0)); // <-- verursacht einen kurzen "offset" in der Kamera beim Drehen
		
		if (ManualCameraBackwardsRotationTimeElapsed != 0 && bManualCameraBackwardsRotationWhenMoving == false)
		{
			ManualCameraBackwardsRotationTimeElapsed = 0;
		}
	}


	// Backwards rotation
	else if ((ManualCameraBackwardsRotationTimeElapsed >= ManualCameraBackwardsRotationTime && bManualCameraBackwardsRotationWhenMoving == false) || (bManualCameraBackwardsRotationWhenMoving == true && bAreActorsToFollowMoving == true))
	{
		if (ManualCameraXValue < 0)
		{
			ManualCameraXValue = ManualCameraXValue + 0.1f * ManualCameraBackwardsRotatingSpeed;
			GetOwner()->AddActorLocalRotation(FRotator(0.1f * ManualCameraBackwardsRotatingSpeed, 0, 0));
			GetOwner()->SetActorRotation(FRotator(GetOwner()->GetActorRotation().Pitch, GetOwner()->GetActorRotation().Yaw, 0)); // <-- verursacht einen kurzen "offset" in der Kamera beim Drehen

			if (ManualCameraXValue < -180)
			{
				ManualCameraXValue += 360;
			}
		}
		else if (ManualCameraXValue > 0)
		{
			ManualCameraXValue = ManualCameraXValue - 0.1f * ManualCameraBackwardsRotatingSpeed;
			GetOwner()->AddActorLocalRotation(FRotator(-0.1f * ManualCameraBackwardsRotatingSpeed, 0, 0));
			GetOwner()->SetActorRotation(FRotator(GetOwner()->GetActorRotation().Pitch, GetOwner()->GetActorRotation().Yaw, 0)); // <-- verursacht einen kurzen "offset" in der Kamera beim Drehen

			if (ManualCameraXValue > 180)
			{
				ManualCameraXValue -= 360;
			}
		}

		if (ManualCameraYValue < 0)
		{
			ManualCameraYValue = ManualCameraYValue + 0.1f * ManualCameraBackwardsRotatingSpeed;
			GetOwner()->AddActorLocalRotation(FRotator(0, 0.1f * ManualCameraBackwardsRotatingSpeed, 0));
			GetOwner()->SetActorRotation(FRotator(GetOwner()->GetActorRotation().Pitch, GetOwner()->GetActorRotation().Yaw, 0)); // <-- verursacht einen kurzen "offset" in der Kamera beim Drehen

			if (ManualCameraYValue < -180)
			{
				ManualCameraYValue += 360;
			}
		}
		else if (ManualCameraYValue > 0)
		{
			ManualCameraYValue = ManualCameraYValue - 0.1f * ManualCameraBackwardsRotatingSpeed;
			GetOwner()->AddActorLocalRotation(FRotator(0, - 0.1f * ManualCameraBackwardsRotatingSpeed, 0));
			GetOwner()->SetActorRotation(FRotator(GetOwner()->GetActorRotation().Pitch, GetOwner()->GetActorRotation().Yaw, 0)); // <-- verursacht einen kurzen "offset" in der Kamera beim Drehen

			if (ManualCameraYValue > 180)
			{
				ManualCameraYValue -= 360;
			}
		}

		if (FMath::IsNearlyZero(ManualCameraXValue, 0.1f * ManualCameraBackwardsRotatingSpeed))
		{
			ManualCameraXValue = 0.0f;
		}

		if (FMath::IsNearlyZero(ManualCameraYValue, 0.1f * ManualCameraBackwardsRotatingSpeed))
		{
			ManualCameraYValue = 0.0f;
		}
	}

	// Count time for backwards rotation if backwards rotation is time dependent
	else if (bManualCameraBackwardsRotationWhenMoving == false)
	{
		ManualCameraBackwardsRotationTimeElapsed += DeltaTime;
	}
	
	
	//UE_LOG(SSCLog, Log, TEXT("Value %f"), GetOwner()->GetInputAxisValue(RotateCameraXAxisMappingName));

	/*if (InputComponentz != nullptr)
	{
		//UE_LOG(SSCLog, Log, TEXT("Value %s"), (GetOwner()->InputComponent->bIsActive ? TEXT("True") : TEXT("False")));
	}
	else
	{
		UE_LOG(SSCLog, Error, TEXT("ManuallyRotateCamera() is called, but InputComponentn is null"));
	}*/
}