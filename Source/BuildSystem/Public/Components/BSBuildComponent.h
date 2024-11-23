#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BSBuildComponent.generated.h"

class UStaticMesh;
class UMaterialInterface;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BUILDSYSTEM_API UBSBuildComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBSBuildComponent();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Build System")
    float UpdateIntervalSeconds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Build System")
    int32 PlayerIndex;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Build System")
    int32 BuildID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Build System")
    float LineTraceDistance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Build System")
    UStaticMesh* TestPreviewMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Build System")
    UMaterialInterface* PreviewValidMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Build System")
    UMaterialInterface* PreviewInvalidMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Build System")
    float AllowedSlopeAngle = 45.0f;

    UPROPERTY(BlueprintReadOnly, Category= "Build System")
    bool bIsBuildModeEnabled;

    UPROPERTY(BlueprintReadOnly, Category= "Build System")
    bool bCanBuild;

    UPROPERTY(BlueprintReadOnly, Category= "Build System")
    FTransform BuildTransform;

    UPROPERTY(BlueprintReadOnly, Category= "Build System")
    UStaticMeshComponent* PreviewMeshComponent;

    UPROPERTY(BlueprintReadOnly, Category= "Build System")
    APlayerCameraManager* PlayerCameraManager;

    UPROPERTY(BlueprintReadOnly, Category= "Build System")
    APawn* PawnOwner;

private:
    FTimerHandle BuildUpdateTimerHandle;

protected:
    virtual void BeginPlay() override;

private:
    void SetupBuildUpdateTimer(float UpdateInterval);
    void UpdateBuildCycle();
    void PerformBuildTrace(bool& bHit, FHitResult& HitResult) const;
    bool CheckBuildConditions(const FHitResult& HitResult) const;

    void SetPreviewMaterialValidity(bool bIsValid);
    void SpawnPreviewMesh();

public:
    UFUNCTION(BlueprintCallable, Category="Build System")
    void SetBuildModeEnabled(const bool bIsEnabled);

    UFUNCTION(BlueprintCallable, Category="Build System")
    void SpawnBuildableActor() const;

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Build System")
    FORCEINLINE bool GetIsBuildModeEnabled() const { return bIsBuildModeEnabled; }
};