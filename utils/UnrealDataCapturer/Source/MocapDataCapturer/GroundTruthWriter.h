#pragma once

#include "CoreMinimal.h"
#include "Engine/SceneCapture2D.h"

#include "Semaphore.h"

#include <thread>

#include "GroundTruthWriter.generated.h"

UCLASS()
class MOCAPDATACAPTURER_API AGroundTruthWriter : public AActor
{
    GENERATED_BODY()

public:
    AGroundTruthWriter();
    ~AGroundTruthWriter();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
        bool isActive;

    virtual void Tick(float DeltaTime) override;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    TArray<FVector> FindGroundTruthMarkers();
    TArray<FVector> foundGroundTruthMarkers;

    float GameTime;

    FString FileName;
    IFileManager* FileManager;
    FILE* FileHandle;

    double RunTime;
};