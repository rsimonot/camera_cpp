PURPLE='\033[1;35m'
BLUE='\033[1;36m'
GREEN='\033[1;32m'
RED='\033[1;31m'
clear

LIBCAMERA_PATH=$(find /usr/local/ -name libcamera.pc)
export PKG_CONFIG_PATH=$LIBCAMERA_PATH
echo -e "${PURPLE}PKG_CONFIG_PATH set !"
echo -e "${PURPLE}-> $PKG_CONFIG_PATH"
echo -e "${BLUE}>"
echo -e "${BLUE}>"
echo -e "${BLUE}>"
echo -e "${BLUE}> Now building ..."
meson build
if [ $? -eq 0 ]; then
    echo -e "${BLUE}>>> Build is ${GREEN}OK${BLUE} !"
    echo -e "${BLUE}>"
    echo -e "${BLUE}>"
    echo -e "${BLUE}>"
    echo -e "${BLUE}> Compiling ..."
    ninja -C build
    if [ $? -eq 0 ]; then
        echo -e "${BLUE}>"
        echo -e "${BLUE}>"
        echo -e "${BLUE}>"
        echo -e "${BLUE}>>> Eveything ${GREEN}OK${BLUE} !"
        echo -e "${BLUE}>>> You can start the app using : [[ ${PURPLE}./build/disocamera${BLUE} ]]  =)"
    else
        echo -e "${BLUE}>>> Compilation ${RED}PROBLEM${BLUE}, take a look at Ninja logs."
    fi
else
    echo -e "${BLUE}>>> Build ${RED}PROBLEM${BLUE}, take a look at Meson logs."
fi