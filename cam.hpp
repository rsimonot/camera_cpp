#include <iostream>
#include <string>
#include <stdint.h>
#include <iomanip>
#include <libcamera/libcamera.h>
#include "file_sink.h"

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
        void requestComplete(libcamera::Request *request);
        void sinkRelease(libcamera::Request *request);

        std::shared_ptr<libcamera::Camera> camera;
        std::unique_ptr<libcamera::ControlList> cameraProperties;
        std::unique_ptr<libcamera::CameraManager> cameraManager;
        std::unique_ptr<libcamera::CameraConfiguration> cameraConfig;
        std::unique_ptr<libcamera::FrameBufferAllocator> cameraAllocator;
        std::unique_ptr<libcamera::Stream> stream;
        std::map<const libcamera::Stream *, std::string> streamNames;
        //std::unique_ptr<libcamera::StreamConfiguration> streamConfig;
        std::vector<std::unique_ptr<libcamera::Request>> requests;
        std::unique_ptr<FileSink> sink;
};

enum {
    option_code_testing     = 0,
    option_code_still       = 1,
    option_code_stream      = 2
};