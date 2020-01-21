/*
	Based on code from Parthiban Nallathambi:

	https://www.linumiz.com/bluetooth-set-adapter-powered-property-and-watch-signal-using-gdbus/
	https://gist.github.com/parthitce/408244ae90a13906a38f5756b0824cb7

	The DBus interface comes as standard in gio:

	sudo apt install libgio3.0-cil-dev

	g++ -std=c++14 -Wall -Wextra -pedantic -O2 $(pkg-config --cflags gio-2.0) example.cpp DBusBluez.cpp $(pkg-config --libs gio-2.0) -o bluez
*/

#include <stdio.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <thread>

#include "DBusBluez.hpp"

namespace {
	volatile std::sig_atomic_t gSignalStatus = 0;

	void signal_handler(int signal)
	{
		gSignalStatus = signal;
	}
}

//
// Utility structure for checking one-shot or periodic events.
//
template<typename T>
struct TimedAlert
{
	T timer = 0;

	// Returns number of timed alerts that elapsed since last call to Test(),
	// attempting to prevent timer drift. Zero timeout = no alert.
	int Test(T dt, T timeout)
	{
		timer += dt;
		if ( (timeout>0) && (timer>=timeout)) {
			int n = timer / timeout; // number of alerts since last call
			timer = timer % timeout; // try to prevent drift
			timer = 0;
			return n;
		}
		return 0;
	}
};


int main(int argc, char **argv)
{
	using IntervalType = uint32_t; // max interval in ms: 2^32 ms = ~50 days

	const auto signal_type = SIGINT;
	const IntervalType timeslice_ms = 10;

	DBusBluez b;
	DBusBluez::Flag flags = DBusBluez::Flags::None;

	IntervalType timeout_ms = 0, info_ms = 5*1000;

	TimedAlert<IntervalType> timeout, info;

	//
	// Set up timeout, if needed.
	//

	if (argc>1)
	{
		double timeout_s = strtod(argv[1], nullptr);
		timeout_ms = (IntervalType)(timeout_s * 1000);
		printf("timeout is %d ms\n", timeout_ms);
	}

	//
	// Signal handler; signal() deprecated, use sigaction() if possible.
	//

	{
		struct sigaction new_action;

		sigemptyset(&new_action.sa_mask);
		new_action.sa_handler = signal_handler;
		new_action.sa_flags = 0;

		if (sigaction(signal_type, &new_action, nullptr) != 0) {
			perror("sigaction: ");
			exit(-1);
		}
	}

	//
	// Start DBus event loop in separate thread
	//

	std::thread thread( [&b,flags] { b.Go(flags); } );

	//
	// Wait until child thread has entered DBus event loop
	//

	while(true) {
		if (g_main_loop_is_running(b.loop) == TRUE) break;
		usleep(1000);		
	}

	//
	// Wait for exit signal, or raise timeout if specified.
	//

	while (gSignalStatus == 0) {
		usleep(timeslice_ms * 1000);

		// Raise signal if timeout>0 and we've exceeded it
		if (timeout.Test(timeslice_ms,timeout_ms)) std::raise(signal_type);

		// Time to output some updated info?
		if (info.Test(timeslice_ms,info_ms)) {
			const std::lock_guard<std::mutex> lock(b.mutex);
			auto now = DBusBluez::Clock::now();
			printf("Devices:\n");
			for (const auto &kv : b.devices) {
				printf("\t%s : %d s\n",
					kv.first.c_str(),
					(int)std::chrono::duration_cast<std::chrono::seconds>(now - kv.second).count()
					);
			}
		}
	}

	//
	// Shut down DBus event loop, join thread.
	//

	printf("Shutting down event loop ...\n");
	if (g_main_loop_is_running(b.loop) == TRUE) {
		g_main_loop_quit(b.loop);
	}

	printf("Joining thread ...\n");
	thread.join();

	return 0;
}
