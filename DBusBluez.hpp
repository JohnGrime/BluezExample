#if !defined(DBUSBLUEZ_INCLUDED)
#define DBUSBLUEZ_INCLUDED

#include <chrono>
#include <map>
#include <mutex>
#include <vector>

#include <gio/gio.h>

//
// Utility routines for Bluez via DBus.
//

struct DBusBluez
{
	//
	// Signature for DBus connection signal callbacks
	//

	using SignalCallback = void (*)(
		GDBusConnection *, // connection
		const gchar *,     // sender name
		const gchar *,     // object path
		const gchar *,     // interface name
		const gchar *,     // signal name
		GVariant *,        // parameters
		gpointer);         // user data

	//
	// For timestamps etc
	//

	using Clock = std::chrono::system_clock;

	//
	// Monitoring flags
	//

	using Flag = uint8_t;

	struct Flags {
		static constexpr Flag None = 0;
		static constexpr Flag PropertyChange = 1<<0;
		static constexpr Flag ObjectManager  = 1<<1;
		static constexpr Flag Firehose = std::numeric_limits<Flag>::max();
	};

	//
	// Some internal state
	//

	GDBusConnection *con;
	GMainLoop *loop = nullptr;
	std::vector<guint> callbacks;

	std::mutex mutex;
	std::map<std::string, Clock::time_point> devices;

	//
	// API
	//

	DBusBluez();
	~DBusBluez();

	void Timestamp(const gchar *obj_id);

	bool SetPowered(bool is_on);
	bool SetDiscovery(bool is_on);

	guint RegisterCallback(
		const gchar *interf_name,
		const gchar *signal_name,
		SignalCallback callback);

	void Go( Flag monitor = Flags::None );
};

#endif
