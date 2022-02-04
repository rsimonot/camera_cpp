#include "cam.hpp"

// Default constructor
CameraDiso::CameraDiso() {}

// Default destructor
CameraDiso::~CameraDiso()
{
	camera->stop();
	cameraAllocator->free(stream.get());
	delete cameraAllocator.get();
	camera->release();
	camera.reset();
	cameraManager->stop();
}

/**
 * @brief Handles request completion events : is called when a Slot connected to the `requestComplete` Signal receives something
 * 
 * @param request The completed request notified on the Signal
 */
void CameraDiso::requestComplete(libcamera::Request *request)
{
	// If the request got cancelled, do nothing
	if (request->status() == libcamera::Request::RequestCancelled)
		return;
	
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
		if (sink) {
			sink = std::make_unique<FileSink>(streamNames);
			sink->configure(*cameraConfig);
			sink->requestProcessed.connect(this, &CameraDiso::sinkRelease);
			sink->processRequest(request);
		}	
   	}
	// case of a stream, the request and associated buffers are reused
	if (!sink) {
		request->reuse(libcamera::Request::ReuseBuffers);
		camera->queueRequest(request);
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
	// Creating a camera manager, that will be able to access cameras
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
	cameraConfig = camera->generateConfiguration( { libcamera::StreamRole::Viewfinder } );

	// Generating Steam configuration for the camera config
	// Once again the first element is a proper default one for us

	streamConfig = std::make_unique<libcamera::StreamConfiguration>(cameraConfig->at(0));

	cameraConfig->validate();		// adjunsting it so it's recognized
	camera->configure(cameraConfig.get());


	/*	====================================
				Preparing the sink
		====================================*/
	streamNames.clear();
	// Filling the map for sink initialization
	for (unsigned int index = 0; index < cameraConfig->size(); ++index) {
		libcamera::StreamConfiguration &cfg = cameraConfig->at(index);
		streamNames[cfg.stream()] = "cam" + cameraId
					   + "-stream" + std::to_string(index);
	}
	/*	==================================== */


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

	// Connecting a Slot to receive the Signals from the camera directly in the app
	camera->requestCompleted.connect(this, &CameraDiso::requestComplete);

	int ret;
	// Starting the "sink" (still don't know how to translate that)
	if (sink) {
		ret = sink->start();
		if (ret) {
			std::cout << "Failed to start frame sink" << std::endl;
			return ret;
		}
	}
	// Starting the camera for real
	ret = camera->start();
	if (ret) {
		std::cout << "Failed to start capture" << std::endl;
		if (sink)
			sink->stop();
		return ret;
	}
	// Iterating through requests to assign them to the camera and then get them back in the "requestComplete" function
	for (std::unique_ptr<libcamera::Request> &request : requests) {
		ret = camera->queueRequest(request.get());
		if (ret < 0) {
			std::cerr << "Can't queue request" << std::endl;
			camera->stop();
			if (sink)
				sink->stop();
			return ret;
		}
	}

	// Cleaning that should happen here has been moved in the destructor, safer due to smart pointers I think
	return 0;
}

void CameraDiso::sinkRelease(libcamera::Request *request)
{
	request->reuse(libcamera::Request::ReuseBuffers);
	camera->queueRequest(request);
}
