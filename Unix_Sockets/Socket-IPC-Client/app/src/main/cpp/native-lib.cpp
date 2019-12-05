#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <memory>
#include "display.h"
#include "Mine/UnixDomainSocket.h"

static IPCUPtr ipc;

void sendColor(uint8_t color) {
	LOGI("** Start send color: %d", color);
  ipc->Send(color, [](std::string_view data)
  {
    LOGI("** Return: %s", (char*)data.data());
  });
  LOGI("** End send color: %d", color);
}

// Handles input touches to the screen
int32_t handle_input(struct android_app* app, AInputEvent* event) {
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		if (AInputEvent_getSource(event) == AINPUT_SOURCE_TOUCHSCREEN) {
			if (AMotionEvent_getAction(event) == AMOTION_EVENT_ACTION_DOWN) {
				float x = AMotionEvent_getX(event,0);
				float y = AMotionEvent_getY(event,0);
				int32_t h_width = ANativeWindow_getWidth(app->window)/2;
				int32_t h_height = ANativeWindow_getHeight(app->window)/2;

				// try to compenstate for the touch screen's home and back button on phone
				if (y > ANativeWindow_getHeight(app->window) * 0.95) {
					return 1;
				}

				LOGI("X: %f  --   Y: %f", x, y);
				
				// Sends color depending on region
				if (x < h_width && y < h_height) { sendColor(0); }
				else if (x > h_width && y < h_height) { sendColor(1); }
				else if (x < h_width && y > h_height) { sendColor(2); }
				else { sendColor(3); }
			}
		}
	}
	return 1;
}

// Main Function
void android_main(struct android_app* app) {

	// Set the callback to process system events
	app->onAppCmd = handle_cmd;
	app->onInputEvent = handle_input;

	// Used to poll the events in the main loop
	int events;
	android_poll_source* source;

	ipc.reset(new IPC(IPC::Client{}));

	// Main loop
	do {
		if (ALooper_pollAll(IsNDKReady() ? 1 : 0, nullptr,
							&events, (void**)&source) >= 0) {
			if (source != NULL) source->process(app, source);
		}
	} while (app->destroyRequested == 0);

	LOGI( "GAME OVER");
}