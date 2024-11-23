#include "Components/BSBuildComponent.h"
#include "Debug/BSLoggerMacros.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceConstant.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"

UBSBuildComponent::UBSBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Set Preview Valid Material
	static ConstructorHelpers::FObjectFinder<UMaterialInstanceConstant> ValidMaterialFinder(
		TEXT("/BuildSystem/Materials/Validity/MI_BSValid.MI_BSValid")
	);
	if (ValidMaterialFinder.Succeeded())
	{
		PreviewValidMaterial = ValidMaterialFinder.Object;
	}
	else
	{
		LOG_ERROR("Failed to find Valid material instance at the specified path.");
	}

	// Set Preview Invalid Material
	static ConstructorHelpers::FObjectFinder<UMaterialInstanceConstant> InvalidMaterialFinder(
		TEXT("/BuildSystem/Materials/Validity/MI_BSInValid.MI_BSInValid")
	);
	if (InvalidMaterialFinder.Succeeded())
	{
		PreviewInvalidMaterial = InvalidMaterialFinder.Object;
	}
	else
	{
		LOG_ERROR("Failed to find Invalid material instance at the specified path.");
	}

	// Set Default Values
	UpdateIntervalSeconds = 0.05f;
	PlayerIndex = 0;
	BuildID = 0;
	LineTraceDistance = 400.0f;
}

void UBSBuildComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(GetOwner()))
	{
		LOG_ERROR("Owner is invalid!")
		return;
	}

	// Get Pawn Owner
	PawnOwner = Cast<APawn>(GetOwner());
	if (!IsValid(PawnOwner))
	{
		LOG_ERROR("Failed to cast Owner to APawn");
		return;
	}

	// Get Player Camera Manager
	if (const UWorld* World = GetOwner()->GetWorld())
	{
		if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
		{
			PlayerCameraManager = PlayerController->PlayerCameraManager;
			if (!IsValid(PlayerCameraManager))
			{
				LOG_ERROR("Failed to get PlayerCameraManager. PlayerController's PlayerCameraManager is invalid!");
			}
		}
		else
		{
			LOG_ERROR("Failed to get PlayerCameraManager. PlayerController is invalid!");
		}
	}
	else
	{
		LOG_ERROR("Failed to get PlayerCameraManager. World is invalid");
	}
}

void UBSBuildComponent::SetupBuildUpdateTimer(float UpdateInterval)
{
	if (UpdateInterval > 0.f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			BuildUpdateTimerHandle,
			this,
			&UBSBuildComponent::UpdateBuildCycle,
			UpdateInterval,
			true
		);
	}
	else
	{
		LOG_ERROR("Invalid update interval or world context for build update timer.");
	}
}

void UBSBuildComponent::UpdateBuildCycle()
{
	// Cast Line Trace
	bool bHit;
	FHitResult HitResult;
	PerformBuildTrace(bHit, HitResult);

	// Calculate Build Transform
	BuildTransform.SetLocation(bHit ? HitResult.ImpactPoint : HitResult.TraceEnd);

	bCanBuild = CheckBuildConditions(HitResult);

	if (!IsValid(PreviewMeshComponent))
	{
		SpawnPreviewMesh();
	}

	// Set Preview Material
	SetPreviewMaterialValidity(bCanBuild);
}

void UBSBuildComponent::PerformBuildTrace(bool& bHit, FHitResult& HitResult) const
{
	if (!IsValid(PlayerCameraManager))
	{
		LOG_ERROR("Failed to perform build trace. PlayerCameraManager is invalid");
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerCameraManager->GetCameraViewPoint(CameraLocation, CameraRotation);

	const FVector Start = CameraLocation;
	const FVector End = Start + (CameraRotation.Vector() * LineTraceDistance);

	FCollisionQueryParams TraceParams(FName(TEXT("BuildTrace")), true, GetOwner());
	TraceParams.bReturnPhysicalMaterial = false;
	TraceParams.AddIgnoredActor(GetOwner());

	bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, TraceParams);
}

bool UBSBuildComponent::CheckBuildConditions(const FHitResult& HitResult) const
{
	if (HitResult.bBlockingHit)
	{
		const float SlopeAngle = FMath::RadiansToDegrees(
			FMath::Acos(FVector::DotProduct(HitResult.ImpactNormal, FVector::UpVector)));
		return (SlopeAngle <= AllowedSlopeAngle);
	}

	return false;
}

void UBSBuildComponent::SetBuildModeEnabled(const bool bIsEnabled)
{
	if (bIsEnabled && (!IsValid(PawnOwner) || !IsValid(PlayerCameraManager)))
	{
		LOG_ERROR("Failed to enable build mode. Owner or PlayerCameraManager is invalid");
		bIsBuildModeEnabled = false;
		return;
	}

	if (bIsEnabled)
	{
		// Start Tick
		SetupBuildUpdateTimer(UpdateIntervalSeconds);
	}
	else
	{
		if (PreviewMeshComponent)
		{
			// Destroy Preview Mesh
			PreviewMeshComponent->DestroyComponent();
			PreviewMeshComponent = nullptr;
		}

		if (GetWorld())
		{
			// Stop Tick
			GetWorld()->GetTimerManager().ClearTimer(BuildUpdateTimerHandle);
		}
	}

	bIsBuildModeEnabled = bIsEnabled;
}

void UBSBuildComponent::SpawnPreviewMesh()
{
	if (!IsValid(PawnOwner))
	{
		LOG_ERROR("Failed to spawn buildable preview. Owner is invalid!");
		return;
	}

	// Add Preview Mesh to Pawn
	PreviewMeshComponent = Cast<UStaticMeshComponent>(
		PawnOwner->AddComponentByClass(UStaticMeshComponent::StaticClass(), false, BuildTransform, false)
	);
	if (!IsValid(PreviewMeshComponent))
	{
		LOG_ERROR("Failed to spawn buildable preview. Failed to add static mesh component to pawn.");
		return;
	}

	if (IsValid(TestPreviewMesh))
	{
		PreviewMeshComponent->SetStaticMesh(TestPreviewMesh);
		PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		LOG_WARN("TestPreviewMesh is invalid!");
	}
}

void UBSBuildComponent::SpawnBuildableActor() const
{
	if (bCanBuild && IsValid(GetWorld()))
	{
		const FVector Location = BuildTransform.GetLocation();
		const FRotator Rotation = BuildTransform.GetRotation().Rotator();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = PawnOwner;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const AStaticMeshActor* BuildableActor = GetWorld()->SpawnActor<AStaticMeshActor>(
			Location, Rotation, SpawnParams);
		if (!IsValid(BuildableActor))
		{
			LOG_ERROR("Failed to spawn buildable actor.");
		}

		if (TestPreviewMesh)
		{
			BuildableActor->GetStaticMeshComponent()->SetStaticMesh(TestPreviewMesh);
		}
		else
		{
			LOG_WARN("TestPreviewMesh is invalid!");
		}

	}
}

void UBSBuildComponent::SetPreviewMaterialValidity(bool bIsValid)
{
	if (!IsValid(PreviewMeshComponent))
	{
		LOG_ERROR("Failed to set preview material. BuildablePreviewMeshComponent is invalid");
		return;
	}

	if (!IsValid(PreviewValidMaterial))
	{
		LOG_ERROR("Failed to set preview material. Valid material is invalid");
		return;
	}

	if (!IsValid(PreviewInvalidMaterial))
	{
		LOG_ERROR("Failed to set preview material. Invalid material is invalid");
		return;
	}

	for (int32 i = 0; i < PreviewMeshComponent->GetNumMaterials(); i++)
	{
		PreviewMeshComponent->SetMaterial(i, bIsValid ? PreviewValidMaterial : PreviewInvalidMaterial);
	}
}