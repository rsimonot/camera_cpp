#include "cam.hpp"

// Default constructor
CameraDiso::CameraDiso() {}

CameraDiso::~CameraDiso() {}
/**
 * @brief Retrieves the camera informations - To be called by other functions of the namespace
 * 
 * @param camera => A camera acquired through a CameraManager
 * @return std::string => The infos about the camera
 */
std::string CameraDiso::getCameraInfos(std::shared_ptr<libcamera::Camera> camera)
{
    cameraProperties = camera->properties();
	std::string name;

	switch (cameraProperties.get(libcamera::properties::Location)) {
	case libcamera::properties::CameraLocationFront:
		name = "Internal front camera";
		break;
	case libcamera::properties::CameraLocationBack:
		name = "Internal back camera";
		break;
	case libcamera::properties::CameraLocationExternal:
		name = "External camera";
		if (cameraProperties.contains(libcamera::properties::Model))
			name += " '" + cameraProperties.get(libcamera::properties::Model) + "'";
		break;
	}

	name += " (" + camera->id() + ")";

	return name;
}

/**
 * @brief Steams a camera. A default camera will automatically be used, as there's only one for now on the system
 * 
 * @return int => Classical return value, 0 means OK
 * 
 * Flow :
 * 1/ Camera Manager
 * 2/ Acquire one particular camera
 * 3/ Generate and validate its config + Configure it
 * 4/ Allocate memory for the streaming (frame buffers)
 * 5/ Create requests for every frame buffer
 */
int8_t CameraDiso::exploitCamera(int8_t option)
{
	// Creating a camera manager, that will be able to access cameras
	//std::unique_ptr<libcamera::CameraManager> cam_mng = std::make_unique<libcamera::CameraManager>();
	cameraManager = std::make_unique<libcamera::CameraManager>();
	cameraManager->start();

	// Selecting camera #0 as default camera
	std::string cameraId = cameraManager->cameras()[0]->id();
	camera = cameraManager->get(cameraId);

	// Control that the camera present on the system is findable
	if (cameraManager->cameras().empty()) {
		std::cout << "ERR : No camera identified on the system." << std::endl;
		cameraManager->stop();
		return 1;
	} else {
		std::cout << " - " << getCameraInfos(camera) << std::endl;
	}

	camera->acquire();

	// Generating camera configuration
	//std::unique_ptr<libcamera::CameraConfiguration> config = camera->generateConfiguration( { libcamera::StreamRole::Viewfinder } );
	cameraConfig = camera->generateConfiguration( { libcamera::StreamRole::Viewfinder } );

	// Generating Steam configuration for the camera config
	// Once again the first element is a proper default one for us

	streamConfig = std::make_unique<libcamera::StreamConfiguration>(cameraConfig->at(0));

	cameraConfig->validate();		// adjunsting it so it's recognized
	camera->configure(cameraConfig.get());

	// The images captured while streaming have to be stored in buffers
	// Using libcamera's FrameBufferAllocator which determines sizes and types on his own
	cameraAllocator = std::make_unique<libcamera::FrameBufferAllocator>(camera);

	// Retrieving the libcamera::Stream associated with the camera in use
	stream = std::make_unique<libcamera::Stream>(*streamConfig->stream());

	//  ~ Control ~
	if (cameraAllocator->allocate(stream.get()) < 0) {
		std::cerr << "ERR : Can't allocate buffers" << std::endl;
		return 2;
	}

	const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &buffers = cameraAllocator->buffers(stream.get());
	// Creating a request for each frame buffer, that'll be queued to the camera, which will then fill it with images
	for (unsigned int i = 0; i < buffers.size(); ++i) {
		std::unique_ptr<libcamera::Request> request = camera->createRequest();	// Initialize a request
		if (!request)
		{
			std::cerr << "ERR : Can't create request" << std::endl;
			return 3;
		}

		const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
		int ret = request->addBuffer(stream.get(), buffer.get());	// Adding current buffer to a request
		if (ret < 0)
		{
			std::cerr << "ERR : Can't set buffer for request"
				  << std::endl;
			return 4;
		}

		// No additionnal controls on the request for now (ex. Brightness...)

		requests.push_back(std::move(request));
	}	// After this loop we got as many <request> objets in "requests" as there were buffers created by the FrameBufferAllocator

	// The camera needs now to fill our buffers

	return 0;
}
