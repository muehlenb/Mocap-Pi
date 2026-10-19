// Fill out your copyright notice in the Description page of Project Settings.

#include "MarkerDetectorCamera.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"

#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h" 	
#include "HAL/FileManagerGeneric.h"

AMarkerDetectorCamera::AMarkerDetectorCamera()
    : FileManager(&FFileManagerGeneric::Get())
    , FileHandle(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
}

AMarkerDetectorCamera::~AMarkerDetectorCamera() {
}

TArray<FVector4f> findMarkersInPointCloud(TArray<FColor> Out, int textureWidth, int textureHeight, int Threshold) {
    // Create scanlines:
    std::vector<std::shared_ptr<ScanRegion>> regions;
    {
        std::shared_ptr<ScanRegion> currentRegion = nullptr;
        unsigned int pcSize = textureWidth * textureHeight;

        for (unsigned int i = 0; i < pcSize; ++i) {
            uint8 irPixel = uint8((int(Out[i].R) + Out[i].G + Out[i].B) / 3);

            if (irPixel > Threshold) {
                if (currentRegion == nullptr)
                    currentRegion = std::make_shared<ScanRegion>(i);

                currentRegion->endIdx = i;
            }
            else {
                if (currentRegion != nullptr) {
                    regions.push_back(currentRegion);
                    currentRegion = nullptr;
                }
            }

            if (i % textureWidth == 0 && currentRegion != nullptr) {
                regions.push_back(currentRegion);
                currentRegion = nullptr;
            }
        }

        if (currentRegion != nullptr) {
            regions.push_back(currentRegion);
            currentRegion = nullptr;
        }
    }

    // Create Regions:
    {
        unsigned int i = 0;
        unsigned int j = 1;

        while (j < regions.size()) {
            std::shared_ptr<ScanRegion> I = regions[i];
            std::shared_ptr<ScanRegion> J = regions[j];
            int iY = I->startIdx / textureWidth;
            int jY = J->startIdx / textureWidth;

            if (iY >= jY) {
                ++j;
            }
            else if (iY + 1 < jY) {
                ++i;
            }
            else {
                int iXStart = I->startIdx % textureWidth;
                int iXEnd = I->endIdx % textureWidth;

                int jXStart = J->startIdx % textureWidth;
                int jXEnd = J->endIdx % textureWidth;

                // if scanlines of consecutive rows are overlapping (touching each other), try to merge:
                if (iXStart <= jXEnd && iXEnd >= jXStart) {
                    regions[i]->Union(regions[j]);
                }

                // if both are in consecutive rows, go further with the scanline which the lower end:
                if (iXEnd < jXEnd)
                    ++i;
                else
                    ++j;
            }
        }
    }

    // Remove non-roots:
    for (unsigned int i = 0; i < regions.size(); ++i) {
        if (regions[i]->parent != nullptr) {
            regions.erase(regions.begin() + i);
            --i;
        }
    }

    TArray<FVector4f> result;
    for (unsigned int i = 0; i < regions.size(); ++i) {
        float x = 0;
        float y = 0;
        float weight = 0;
        int pxCount = 0;

        // Add all children:
        for (std::shared_ptr<ScanRegion>& r : regions[i]->childs) {
            for (int h = r->startIdx; h <= r->endIdx; ++h) {
                float ir = (((int(Out[h].R) + Out[h].G + Out[h].B) / 3) / float(Threshold));
                x += (h % textureWidth) * ir;
                y += (h / textureWidth) * ir;
                weight += ir;
                ++pxCount;
            }
        }

        // Add self:
        for (int h = regions[i]->startIdx; h <= regions[i]->endIdx; ++h) {
            float ir = (((int(Out[h].R) + Out[h].G + Out[h].B) / 3) / float(Threshold));
            x += (h % textureWidth) * ir;
            y += (h / textureWidth) * ir;
            weight += ir;
            ++pxCount;
        }

        result.Add(FVector4f(x,y,float(pxCount), weight));
    }

    return result;
}

void AMarkerDetectorCamera::BeginPlay() {
	ASceneCapture2D::BeginPlay();
    RunTime = 0;

	TextureTarget = createTextureRenderTarget2D();
	USceneCaptureComponent2D* camera = GetCaptureComponent2D();
	camera->TextureTarget = TextureTarget;
	camera->FOVAngle = FOVAngle;
	camera->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

	RenderTargetResource = (FTextureRenderTarget2DResource*)TextureTarget->GetResource();

	Flags = FReadSurfaceDataFlags(RCM_MinMax, CubeFace_PosX);
	Flags.SetLinearToGamma(false);

	Rect = FIntRect(0, 0, textureWidth, textureHeight);

    runProcessingThread = true;

    processingThread = std::thread([this]() {
        FileName = FPaths::ProjectDir() / TEXT("recordings") / FString::FromInt(camId) + ".txt";

        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        FString DirectoryPath = FPaths::GetPath(FileName);
        if (!PlatformFile.DirectoryExists(*DirectoryPath))
        {
            PlatformFile.CreateDirectoryTree(*DirectoryPath);
        }

        FileHandle = fopen(TCHAR_TO_ANSI(*FileName), "w");
        if (FileHandle != nullptr)
        {
            FString TextToWrite = TEXT("Cam Recording\n");
            fputs(TCHAR_TO_ANSI(*TextToWrite), FileHandle);
        }

        while (runProcessingThread) {
            semaphoreImageAvailable.acquire();

            if (!runProcessingThread)
                break;

            double startTime = FPlatformTime::Seconds();
            TArray<FVector4f> markers = findMarkersInPointCloud(OutData, textureWidth, textureHeight, Threshold);

            FString TextToWrite = FString::Printf(TEXT("%f"), GameTime);
            fputs(TCHAR_TO_ANSI(*TextToWrite), FileHandle);
            for (FVector4f marker : markers) {
                TextToWrite = FString::Printf(TEXT(",[%f,%f,%f,%f]"), marker.X, marker.Y, marker.Z, marker.W);
                fputs(TCHAR_TO_ANSI(*TextToWrite), FileHandle);

                // UE_LOG(LogTemp, Warning, TEXT("Marker Size: %f"), marker.Z);
            }

            TextToWrite = FString::Printf(TEXT("\n"));
            fputs(TCHAR_TO_ANSI(*TextToWrite), FileHandle);

            double endTime = FPlatformTime::Seconds();
            //UE_LOG(LogTemp, Warning, TEXT(" \t Found %i Markers in %f Secs"), markers.Num(), endTime - startTime);

            semaphoreImageProcessed.release();
        }

        // Close the file
        if (FileHandle != nullptr)
        {
            fclose(FileHandle);
            FileHandle = nullptr;
        }
    });
}


void AMarkerDetectorCamera::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    ASceneCapture2D::EndPlay(EndPlayReason);

    runProcessingThread = false;
    semaphoreImageAvailable.release();
    processingThread.join();
}

void AMarkerDetectorCamera::Tick(float DeltaTime) {
    ASceneCapture2D::Tick(DeltaTime);
    RunTime += 0.01666666666666667;

    USceneCaptureComponent2D* camera = GetCaptureComponent2D();
    camera->bCaptureEveryFrame = isActive;
    camera->bCaptureOnMovement = isActive;

    double tmpGameTime = RunTime;

    if (FrameCounter % EveryNRecording == ModFrame){
        ENQUEUE_RENDER_COMMAND(ReadSurfaceCommand)(
            [this, tmpGameTime](FRHICommandListImmediate& RHICmdList)
            {
                double startTime = FPlatformTime::Seconds();
                semaphoreImageProcessed.acquire();

                GameTime = tmpGameTime;
                RHICmdList.ReadSurfaceData(
                    RenderTargetResource->GetRenderTargetTexture(),
                    Rect,
                    OutData,
                    Flags
                );

                semaphoreImageAvailable.release();

                double endTime = FPlatformTime::Seconds();
                UE_LOG(LogTemp, Warning, TEXT("RGB Sensor (Read Surface): %f"), endTime - startTime);
            }
        );
    }
    ++FrameCounter;
}

UTextureRenderTarget2D* AMarkerDetectorCamera::createTextureRenderTarget2D() {
	UTextureRenderTarget2D* textureTarget = NewObject<UTextureRenderTarget2D>();
	textureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
	textureTarget->bAutoGenerateMips = false;
	textureTarget->bGPUSharedFlag = true;
	textureTarget->InitAutoFormat(textureWidth, textureHeight);
	textureTarget->UpdateResourceImmediate();
	textureTarget->TargetGamma = 1.0f;
	return textureTarget;
}