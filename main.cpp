#include "cam.hpp"

int main (void)
{
    CameraDiso *cam = new CameraDiso();

    int8_t res = cam->exploitCamera(option_code_testing);
    cam->~CameraDiso();

    if (res == 0)
        return EXIT_SUCCESS;
    else {
        std::cout << "exploitCamera exited with error code : " << res << std::endl;
        return EXIT_FAILURE;
    }
}