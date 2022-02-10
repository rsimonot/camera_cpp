#include "cam.hpp"

int main (void)
{
    CameraDiso *cam = new CameraDiso();
    int res;
    /*
    if (res != 0) {
        std::cout << "\033[1;31mexploitCamera exited with error code : \033[0m" << res << std::endl;
        return EXIT_FAILURE;
    }
    */
    std::cout << "\033[0;36m.+* EXPLOIT WITH OPTION STILL *+.\033[0m" << std::endl;
    res = cam->exploitCamera(option_code_still);

    if (res == 0)
        return EXIT_SUCCESS;
    else {
        std::cout << "\033[1;31mexploitCamera exited with error code : \033[0m" << res << std::endl;
        return EXIT_FAILURE;
    }
}