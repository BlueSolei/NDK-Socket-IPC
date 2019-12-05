#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include "display.h"
#include "Mine/UnixDomainSocket.h"

// Main function
void android_main(struct android_app* app) {

	// Set the callback to process system events
	app->onAppCmd = handle_cmd;

	// Used to poll the events in the main loop
	int events;
	android_poll_source* source;

	setColorSelected(0); // Start with red

	char buffer[BUFFER_SIZE];
	// Start server daemon on new thread
	IPC ipc(IPC::Server(), [&](std::string_view data) {
		LOGI("** Server got data: %d", data[0]);
		setColorSelected(data[0]);
		// Send back result
		sprintf((char*)buffer, "%d", "Got message");
		return std::string_view(buffer);
	});

	// Main loop
	do {
		if (ALooper_pollAll(IsNDKReady() ? 1 : 0, nullptr,
							&events, (void**)&source) >= 0) {
			if (source != NULL) source->process(app, source);
		}
	} while (app->destroyRequested == 0);

	LOGI("GAME OVER");
}


