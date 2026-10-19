#pragma once

#include "CoreMinimal.h"
#include "Engine/SceneCapture2D.h"

#include "Semaphore.h"

#include <thread>

#include "MarkerDetectorCamera.generated.h"


struct ScanRegion : public std::enable_shared_from_this<ScanRegion> {
    std::shared_ptr<ScanRegion> parent;

    std::vector<std::shared_ptr<ScanRegion>> childs;

    int startIdx;
    int endIdx;

    int depth;

    ScanRegion(int idx)
        : parent(nullptr)
        , startIdx(idx)
        , endIdx(idx)
        , depth(1)
    {
        //childsInclSelf.push_back(shared_from_this());
    }

    std::shared_ptr<ScanRegion> getRoot() {
        return getRoot(nullptr, nullptr);
    }

    std::shared_ptr<ScanRegion> getRoot(std::shared_ptr<ScanRegion> previous, std::shared_ptr<ScanRegion> prePrevious) {
        std::shared_ptr<ScanRegion> self = shared_from_this();

        // path compression:
        if (prePrevious != nullptr) {
            prePrevious->parent = self;
        }

        if (self->parent == nullptr)
            return self;

        return parent->getRoot(self, previous);
    }

    void Union(std::shared_ptr<ScanRegion> node) {
        std::shared_ptr<ScanRegion> aRoot = getRoot(nullptr, nullptr);
        std::shared_ptr<ScanRegion> bRoot = node->getRoot(nullptr, nullptr);

        if (aRoot == bRoot)
            return;

        // attach tree with smaller depth to the tree with higher depth:
        if (aRoot->depth < bRoot->depth) {
            aRoot->parent = bRoot;
            bRoot->depth += aRoot->depth;
            bRoot->childs.insert(bRoot->childs.end(), aRoot->childs.begin(), aRoot->childs.end());
            bRoot->childs.push_back(shared_from_this());
        }
        else {
            bRoot->parent = aRoot;
            aRoot->depth += bRoot->depth;
            aRoot->childs.insert(aRoot->childs.end(), bRoot->childs.begin(), bRoot->childs.end());
            aRoot->childs.push_back(node);
        }
    }
};



/**
 * 
 */
UCLASS()
class MOCAPDATACAPTURER_API AMarkerDetectorCamera : public ASceneCapture2D
{
	GENERATED_BODY()

public:
	AMarkerDetectorCamera();
	~AMarkerDetectorCamera();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
        bool isActive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sensor")
		UTextureRenderTarget2D* TextureTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
        int camId = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
        int textureWidth = 1280;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
		int textureHeight = 720;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
        float FOVAngle = 62;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
        int Threshold = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
        int EveryNRecording = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
        int ModFrame = 0;

    int FrameCounter = 0;

	virtual void Tick(float DeltaTime) override;

	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UTextureRenderTarget2D* createTextureRenderTarget2D();

	FRenderTarget* RenderTargetResource;
	TArray<FColor> OutData;
	FIntRect Rect;
	FReadSurfaceDataFlags Flags;

    bool runProcessingThread = false;
    std::thread processingThread;
    Semaphore semaphoreImageAvailable = Semaphore(0);
    Semaphore semaphoreImageProcessed = Semaphore(1);

    FString FileName;
    IFileManager* FileManager;
    FILE* FileHandle;

    double RunTime;
    double GameTime = 0;
};
