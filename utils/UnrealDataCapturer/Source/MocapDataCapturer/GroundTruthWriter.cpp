#include "GroundTruthWriter.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h" 	
#include "HAL/FileManagerGeneric.h"

AGroundTruthWriter::AGroundTruthWriter()
    : FileManager(&FFileManagerGeneric::Get())
    , FileHandle(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
}

AGroundTruthWriter::~AGroundTruthWriter() {
}

void AGroundTruthWriter::BeginPlay() {
    AActor::BeginPlay();
    RunTime = 0;

    FileName = FPaths::ProjectDir() / TEXT("recordings") / "GroundTruth3D.txt";

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    FString DirectoryPath = FPaths::GetPath(FileName);
    if (!PlatformFile.DirectoryExists(*DirectoryPath))
    {
        PlatformFile.CreateDirectoryTree(*DirectoryPath);
    }

    FileHandle = fopen(TCHAR_TO_ANSI(*FileName), "w");
    if (FileHandle != nullptr)
    {
        FString TextToWrite = TEXT("Ground Truth Recording\n");
        fputs(TCHAR_TO_ANSI(*TextToWrite), FileHandle);
    }
}


void AGroundTruthWriter::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    AActor::EndPlay(EndPlayReason);

    if (FileHandle != nullptr)
    {
        fclose(FileHandle);
        FileHandle = nullptr;
    }
}

TArray<FVector> AGroundTruthWriter::FindGroundTruthMarkers() {
    TArray<FVector> groundTruthMarkers;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);

    int i = 0; 
    for (AActor* actor : FoundActors) {
        if (actor == nullptr || actor->IsHidden())
            continue;

        for (UActorComponent* comp : actor->GetComponents()) {
            if (UStaticMeshComponent* meshComp = Cast<UStaticMeshComponent>(comp)) {
                FVector pos = meshComp->GetComponentLocation();
                
                if(meshComp->GetStaticMesh().GetName().Equals(FString("Sphere"))){
                    groundTruthMarkers.Add(pos);
                    ++i;
                }
            }
        }
    }

    foundGroundTruthMarkers = groundTruthMarkers;

    return groundTruthMarkers;
}

void AGroundTruthWriter::Tick(float DeltaTime) {
	AActor::Tick(DeltaTime);
    RunTime += 0.01666666666666667;

    double startTime = FPlatformTime::Seconds();
    TArray<FVector> markers = FindGroundTruthMarkers();

    FString TextToWrite = FString::Printf(TEXT("%f"), RunTime);
    fputs(TCHAR_TO_ANSI(*TextToWrite), FileHandle);
    for (FVector marker : markers) {
        FVector scaledMarker = marker / 100;
        TextToWrite = FString::Printf(TEXT(",[%f,%f,%f,1.0]"), scaledMarker.X, scaledMarker.Z, scaledMarker.Y);
        fputs(TCHAR_TO_ANSI(*TextToWrite), FileHandle);
    }

    TextToWrite = FString::Printf(TEXT("\n"));
    fputs(TCHAR_TO_ANSI(*TextToWrite), FileHandle);

    double endTime = FPlatformTime::Seconds();
    UE_LOG(LogTemp, Warning, TEXT(" \t Found %i Markers in %f Secs"), markers.Num(), endTime - startTime);
}