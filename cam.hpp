#include <iostream>
#include <string>
#include <stdint.h>
#include <libcamera/libcamera.h>

class CameraDiso
{
    public:
        CameraDiso();
        virtual ~CameraDiso();
        int8_t exploitCamera(int8_t option);

    protected:
        int8_t option;

    private:
        std::string getCameraInfos(std::shared_ptr<libcamera::Camera> camera);
        std::shared_ptr<libcamera::Camera> camera;
        libcamera::ControlList cameraProperties;
        std::unique_ptr<libcamera::CameraManager> cameraManager;
        std::unique_ptr<libcamera::CameraConfiguration> cameraConfig;
        std::unique_ptr<libcamera::FrameBufferAllocator> cameraAllocator;
        std::unique_ptr<libcamera::Stream> stream;
        std::unique_ptr<libcamera::StreamConfiguration> streamConfig;
        std::vector<std::unique_ptr<libcamera::Request>> requests;
};

enum {
    option_code_testing     = 0,
    option_code_still       = 1,
    option_code_stream      = 2
};