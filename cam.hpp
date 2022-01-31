#include <iostream>
#include <string>
#include <stdint.h>
#include <libcamera/libcamera.h>

class CameraDiso
{
    public:
        CameraDiso();
        virtual ~CameraDiso();

    protected:
        int8_t option;

    private:
        std::string getCameraInfos(libcamera::Camera *camera);
        int8_t exploitCamera(int8_t option);
        std::shared_ptr<libcamera::Camera> camera;
        const libcamera::ControlList &cameraProperties;
        std::unique_ptr<libcamera::CameraManager> cameraManager;
        std::unique_ptr<libcamera::CameraConfiguration> cameraConfig;
        std::unique_ptr<libcamera::FrameBufferAllocator> cameraAllocator;
        std::unique_ptr<libcamera::Stream> stream;
        std::unique_ptr<libcamera::StreamConfiguration> streamConfig;
        std::vector<std::unique_ptr<libcamera::FrameBuffer>> &buffers;
        std::vector<std::unique_ptr<libcamera::Request>> requests;
}