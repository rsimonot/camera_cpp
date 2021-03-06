#include <iostream>                     // std::cout ; std::endl
#include <string>
#include <stdint.h>                     // int8_t
#include <iomanip>                      // std::setw ; std::setfill
#include <functional>                   // std::bind
#include <libcamera/libcamera.h>
#include <jpeglib.h>
#include "file_sink.h"
#include "event_loop.h"

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
        static void processRequest(libcamera::Request *request, CameraDiso *instance);
        void sinkRelease(libcamera::Request *request);
        void make_jpeg(const libcamera::FrameMetadata *metadata);

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

        EventLoop loop;
        uint8_t* jpeg_buffer;
        unsigned long jpeg_len;
};

enum {
    option_code_testing     = 0,
    option_code_still       = 1,
    option_code_stream      = 2,
    option_code_sink        = 3
};