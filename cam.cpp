#include "cam.hpp"

// Default constructor
CameraDiso::CameraDiso() {}

// Default destructor
// Probably there's nothing to put in it as I'm using smart pointers
// For now I prefer letting the actions on camera and cameraManager
CameraDiso::~CameraDiso()
{
	camera->stop();
	camera->release();
	camera.reset();
	cameraManager->stop();
}



/**
 * @brief Handles request completion events : is called when a Slot connected to the `requestComplete` Signal receives something
 * 
 * @param request the completed request notified by the signal
 */
void CameraDiso::requestComplete(libcamera::Request *request)
{
	//std::cout << "\033[1;33m###### Entering 'requestComplete' function\033[0m" << std::endl;

	// If the request got cancelled, do nothing
	if (request->status() == libcamera::Request::RequestCancelled)
		return;

	loop.callLater(std::bind(CameraDiso::processRequest, request, this));
}

/**
 * @brief !STATIC! Called during request completion events by the event loop
 * 
 * @param request the completed request notified by the signal
 * @param instance the current CameraDiso instance, trick to manipulate <this> in a static member function
 */
void CameraDiso::processRequest(libcamera::Request *request, CameraDiso *instance)
{
	//std::cout << "\033[1;33m###### Entering 'processRequest' function\033[0m" << std::endl;
	
	// If the request was treated, the output data is in a map of Streams and Buffers
	const libcamera::Request::BufferMap &buffers = request->buffers();
	// Iterating through those buffers
	for (auto bufferPair : buffers) {
    	libcamera::FrameBuffer *buffer = bufferPair.second;
    	const libcamera::FrameMetadata &metadata = buffer->metadata();	// retrieving metadatas for instance
		// Displaying informations about them to trace camera activity
		std::cout << " seq: " << std::setw(6) << std::setfill('0') << metadata.sequence << " bytesused: ";
		unsigned int nplane = 0;
		for (const libcamera::FrameMetadata::Plane &plane : metadata.planes())
		{
			std::cout << plane.bytesused;
			if (++nplane < metadata.planes().size()) std::cout << "/";
		}
		std::cout << std::endl;

		// Sink enables to write image data to disk ?
		// For now there's now interractivity, it'll have to be introduced at the same time as gRPC
		if (instance->option == option_code_still) {
			instance->sink = std::make_unique<FileSink>(instance->streamNames, "test/");
			instance->sink->configure(*instance->cameraConfig.get());
			instance->sink->requestProcessed.connect(instance, &CameraDiso::sinkRelease);
			instance->sink->processRequest(request);
		}	
   	}
	// case of a stream, the request and associated buffers are reused
	if (!instance->sink) {
		request->reuse(libcamera::Request::ReuseBuffers);
		instance->camera->queueRequest(request);
	}
}

/**
 * @brief Retrieves the camera informations - To be called by other functions of the namespace
 * 
 * @param camera A camera acquired through a CameraManager
 * @return std::string name => The infos about the camera
 */
std::string CameraDiso::getCameraInfos(std::shared_ptr<libcamera::Camera> camera)
{
	std::cout << "\033[1;35m###### Camera Infos >>> \033[0m";
    cameraProperties = std::make_unique<libcamera::ControlList>(camera->properties());
	std::string name;

	switch (cameraProperties.get()->get(libcamera::properties::Location)) {
	case libcamera::properties::CameraLocationFront:
		name = "Internal front camera";
		break;
	case libcamera::properties::CameraLocationBack:
		name = "Internal back camera";
		break;
	case libcamera::properties::CameraLocationExternal:
		name = "External camera";
		if (cameraProperties.get()->contains(libcamera::properties::Model))
			name += " '" + cameraProperties.get()->get(libcamera::properties::Model) + "'";
		break;
	}

	name += " (" + camera->id() + ")";

	return name;
}

/**
 * @brief Steams a camera. A default camera will automatically be used, as there's only one for now on the system
 * 
 * @return <int> Classical return value, 0 means OK
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
	this->option = option;
	// Creating a camera manager, that will be able to access cameras
	cameraManager = std::make_unique<libcamera::CameraManager>();
	cameraManager->start();
	std::cout << "\033[1;35m###### Started Camera Manager\033[0m" << std::endl;

	// Selecting camera #0 as default camera
	std::string cameraId = cameraManager->cameras()[0]->id();
	camera = cameraManager->get(cameraId);

	// Control that the camera present on the system is findable
	if (cameraManager->cameras().empty()) {
		std::cout << "\033[1;31m###### ERR : No camera identified on the system\033[0m" << std::endl;
		cameraManager->stop();
		return 1;
	} else {
		std::cout << " - " << getCameraInfos(camera) << std::endl;
	}

	camera->acquire();
	std::cout << "\033[1;35m###### Camera acquired\033[0m" << std::endl;

	// Generating camera configuration
	cameraConfig = camera->generateConfiguration( { libcamera::StreamRole::StillCapture } );

	cameraConfig->at(0).pixelFormat = libcamera::formats::YUV420;
	cameraConfig->at(0).size.width = 800;
	cameraConfig->at(0).size.height = 600;
	cameraConfig->at(0).colorSpace = libcamera::ColorSpace::Jpeg;	// works eventhough VS Code is doesn't recognize it
	cameraConfig->validate();		// adjunsting it so it's recognized
	camera->configure(cameraConfig.get());
	std::cout << "\033[1;35m###### Camera configured\033[0m" << std::endl;
	
	/*	====================================
				Preparing the sink
		====================================*/
	streamNames.clear();
	// Filling the map for sink initialization
	std::cout << "\033[1;35m###### Preparing the sink, registered in <streamNames> :\033[0m" << std::endl;
	for (unsigned int index = 0; index < cameraConfig->size(); ++index) {
		libcamera::StreamConfiguration &cfg = cameraConfig->at(index);
		streamNames[cfg.stream()] = "cam" + cameraId
					   + "-stream" + std::to_string(index);
		std::cout << "cam" + cameraId + "-stream" + std::to_string(index) << std::endl;
	}
	/*	==================================== */


	// The images captured while streaming have to be stored in buffers
	// Using libcamera's FrameBufferAllocator which determines sizes and types on his own
	cameraAllocator = std::make_unique<libcamera::FrameBufferAllocator>(camera);
	std::cout << "\033[1;35m###### <cameraAllocator> OK\033[0m" << std::endl;

	// Stream config is no longer a class member as we need a reference
	// I'll have to write that in a better way later on
	libcamera::StreamConfiguration &streamConfig = cameraConfig->at(0);
	// Retrieving the libcamera::Stream associated with the camera in use
	//stream = std::make_unique<libcamera::Stream>(streamConfig.stream());
	std::cout << "\033[1;35m###### <streamConfig> OK\033[0m" << std::endl;
	//  ~ Control ~
	if (cameraAllocator->allocate(streamConfig.stream()) < 0) {
		std::cerr << "\033[1;31m###### ERR : Can't allocate buffers\033[0m" << std::endl;
		return 2;
	}
	std::cout << "\033[1;35m###### Allocated frame buffers\033[0m" << std::endl;

	const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &buffers = cameraAllocator->buffers(streamConfig.stream());
	// Creating a request for each frame buffer, that'll be queued to the camera, which will then fill it with images
	for (unsigned int i = 0; i < buffers.size(); ++i) {
		std::unique_ptr<libcamera::Request> request = camera->createRequest();	// Initialize a request
		if (!request)
		{
			std::cerr << "\033[1;31m###### ERR : Can't create request\033[0m" << std::endl;
			return 3;
		}

		const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
		int ret = request->addBuffer(streamConfig.stream(), buffer.get());	// Adding current buffer to a request
		if (ret < 0)
		{
			std::cerr << "\033[1;31m###### ERR : Can't set buffer for request\033[0m"
				  << std::endl;
			return 4;
		}

		// No additionnal controls on the request for now (ex. Brightness...)

		requests.push_back(std::move(request));
	}	// After this loop we got as many <request> objets in "requests" as there were buffers created by the FrameBufferAllocator
	std::cout << "\033[1;35m###### Filled <requests>\033[0m" << std::endl;

	// Connecting a Slot to receive the Signals from the camera directly in the app
	camera->requestCompleted.connect(this, &CameraDiso::requestComplete);
	std::cout << "\033[1;35m###### Connected to requestCompleted\033[0m" << std::endl;

	int ret;
	// Starting the "sink" (still don't know how to translate that)
	if (sink) {
		ret = sink->start();
		std::cout << "\033[1;35m###### Sink started\033[0m" << std::endl;
		if (ret) {
			std::cout << "Failed to start frame sink" << std::endl;
			return 5;
		}
	}
	// Starting the camera for real
	ret = camera->start();
	if (ret) {
		std::cout << "Failed to start capture" << std::endl;
		if (sink)
			sink->stop();
		return 6;
	} else {
		std::cout << "\033[1;35m###### Camera started\033[0m" << std::endl;
	}
	// Iterating through requests to assign them to the camera and then get them back in the "requestComplete" function
	for (std::unique_ptr<libcamera::Request> &request : requests) {
		ret = camera->queueRequest(request.get());
		std::cout << "\033[1;35m###### queued Request :  \033[0m" << request->toString() << std::endl;
		if (ret < 0) {
			std::cerr << "Can't queue request" << std::endl;
			camera->stop();
			if (sink)
				sink->stop();
			return 7;
		}
	}

	loop.timeout(3);	// Preparing to capture for 3 seconds
	std::cout << "\033[1;35m###### Loop Timeout OK\033[0m" << std::endl;
	ret = loop.exec();
	std::cout << "\033[1;33m###### Capture exited with status : \033[0m" << ret << std::endl;

	std::cout << "\033[1;35m###### All work done !\033[0m" << std::endl;
	
	// Cleaning that should happen here has been moved in the destructor, safer due to smart pointers I think
	return 0;
}

void CameraDiso::sinkRelease(libcamera::Request *request)
{
	request->reuse(libcamera::Request::ReuseBuffers);
	camera->queueRequest(request);
}
