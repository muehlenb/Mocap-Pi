#include "src/PiCamera.h"

#include <functional>

#include "Vec4f.h"

using namespace libcamera;

void PiCamera::start(){
    cm->start();

    // List all available cameras (expect only one):
    for (auto const &camera : cm->cameras())
        std::cout << camera->id() << std::endl;

    // If no camera available, exit:
    if (cm->cameras().empty()) {
        std::cout << "No cameras were identified on the system." << std::endl;
        cm->stop();
        return;
    }

    // Get the first camera:
    std::string cameraId = cm->cameras()[0]->id();
    camera = cm->get(cameraId);

    camera->acquire();

    // Examine control elements:
    auto controls = camera->controls();
    for (const auto &control : controls) {
        std::cout << control.first->name() << std::endl;
    }

    // Configure:
    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration( { StreamRole::Viewfinder } );
    StreamConfiguration &streamConfig = config->at(0);
    std::cout << "Default viewfinder configuration is: " << streamConfig.toString() << std::endl;
    for (const auto &format : streamConfig.formats().pixelformats()) {
        std::cout << "Unterstütztes Format: " << format.toString() << std::endl;
    }

    streamConfig.size.width = 1280;
    streamConfig.size.height = 720;
    streamConfig.pixelFormat = PixelFormat::fromString("YUV420");

//    streamConfig.pixelFormat = ...; // Spezifisches Format für Rot-Kanal
//    streamConfig.colorSpace = ...;  // Spezifischer Farbraum, falls erforderlich

    config->validate();
    std::cout << "Validated viewfinder configuration is: " << streamConfig.toString() << std::endl;

    camera->configure(config.get());

    allocator = new FrameBufferAllocator(camera);
    for (StreamConfiguration &cfg : *config) {
        int ret = allocator->allocate(cfg.stream());
        if (ret < 0) {
            std::cerr << "Can't allocate buffers" << std::endl;
            return;
        }

        size_t allocated = allocator->buffers(cfg.stream()).size();
        std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
    }

    stream = streamConfig.stream();
    const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);

    for (unsigned int i = 0; i < buffers.size(); ++i) {
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request)
        {
            std::cerr << "Can't create request" << std::endl;
            return;
        }

        const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
        int ret = request->addBuffer(stream, buffer.get());
        if (ret < 0)
        {
            std::cerr << "Can't set buffer for request" << std::endl;
            return;
        }

        requests.push_back(std::move(request));
    }

    // Register callback:
    std::function<void(libcamera::Request*)> func = std::bind(&PiCamera::requestComplete, this, std::placeholders::_1);

    camera->requestCompleted.connect(this, &PiCamera::requestComplete);

    camera->start();
    for (std::unique_ptr<Request> &request : requests){
        std::int64_t frameDuration = std::int64_t(1000000 / framerate);

        std::int64_t value_pair[2] = {frameDuration,frameDuration};
        request->controls().set(libcamera::controls::FrameDurationLimits, libcamera::Span<const std::int64_t, 2>(value_pair));
        request->controls().set(libcamera::controls::ExposureTime, exposureTime);
        request->controls().set(libcamera::controls::LensPosition, lensPosition);
        request->controls().set(libcamera::controls::AfMode, 0);
        request->controls().set(libcamera::controls::AeEnable, false);
        camera->queueRequest(request.get());
    }
}

void PiCamera::stop(){
    camera->stop();
    allocator->free(stream);
    delete allocator;
    camera->release();
    camera.reset();
    cm->stop();
}

void PiCamera::registerImageCallback(std::function<void(void*, unsigned int, unsigned int, unsigned int, unsigned int, uint64_t)> cb){
    imageCallback = cb;
}

/**
* Processes a frame of the camera
*/
void PiCamera::requestComplete(Request *request){
    if (request->status() == Request::RequestCancelled)
        return;


    const std::map<const Stream *, FrameBuffer *> &buffers = request->buffers();
    for (auto bufferPair : buffers) {
        FrameBuffer *buffer = bufferPair.second;
        const FrameMetadata &metadata = buffer->metadata();

        const auto &plane = buffer->planes()[0];
        void *data = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), plane.offset);
        if (data != MAP_FAILED) {
            // Measure time and identify frame drops:
            {
                measuredFramerate = 1000000000.0 / (metadata.timestamp - lastMeasuredTimestamp);
                lastMeasuredTimestamp = metadata.timestamp;

                frameDropBuffer[frameDropBufferIdx++] = measuredFramerate * 1.05 < framerate;
                if(frameDropBufferIdx > 100)
                    frameDropBufferIdx = 0;
            }

            if(imageCallback)
                imageCallback(data, plane.length, 1280, 720, metadata.sequence, metadata.timestamp);
            else 
                munmap(data, plane.length);
        } else {
            munmap(data, plane.length);
        }
    }

    request->reuse(Request::ReuseBuffers);
    std::int64_t frameDuration = std::int64_t(1000000 / framerate);

    std::int64_t value_pair[2] = {frameDuration,frameDuration};
    request->controls().set(libcamera::controls::FrameDurationLimits, libcamera::Span<const std::int64_t, 2>(value_pair));
    request->controls().set(libcamera::controls::ExposureTime, exposureTime);
    request->controls().set(libcamera::controls::LensPosition, lensPosition);
    camera->queueRequest(request);
}
