#include <stdio.h>
#include <unistd.h>

#include "DBusBluez.hpp"

//
// Static utility routines for use with DBus
//

///*
static void gvariant_print(const char *prefix, const gchar *key, GVariant *value)
{
	const gchar *type = g_variant_get_type_string(value), *uuid;
	GVariantIter i;

	printf("%s%s : ", prefix, key);
	switch(*type) {
		case 'b':
			printf("%d (%s)\n", g_variant_get_boolean(value), type);
			break;

		// actually unsigned char, but no explicit "variant get" method for u8!
		case 'y':
			printf("%d (%s)\n", g_variant_get_byte(value), type);
			break;

		case 'n':
			printf("%d (%s)\n", g_variant_get_int16(value), type);
			break;

		case 'q':
			printf("%d (%s)\n", g_variant_get_uint16(value), type);
			break;

		case 'i':
		case 'h':
			printf("%d (%s)\n", g_variant_get_int32(value), type);
			break;

		case 'u':
			printf("%d (%s)\n", g_variant_get_uint32(value), type);
			break;

		case 'o':
		case 's':
			printf("%s (%s)\n", g_variant_get_string(value, NULL), type);
			break;

		case 'a':
			if (strcmp(type, "as") == 0) {
				printf("\n");
				g_variant_iter_init(&i, value);
				while(g_variant_iter_next(&i, "s", &uuid)) {
					printf("%s\t%s\n", prefix, uuid);
				}
			} else printf("Other (%s)\n", type);
			break;

		default:
			printf("Other (%s)\n", type);
			break;
	}
}

static bool check_signature(
	GVariant *var,
	const gchar *expect,
	const gchar *signal)
{
	const gchar *sig = g_variant_get_type_string(var);
	if(g_strcmp0(sig, expect) != 0) {
		printf("Invalid signature for %s: %s != %s", signal, sig, expect);
		return false;
	}
	return true;
}
//*/

static void cb_firehose(
	GDBusConnection *,
	const gchar *sender_name,
	const gchar *object_path,
	const gchar *interface,
	const gchar *signal_name,
	GVariant *,
	gpointer user_data)
{
	printf("FH: sender='%s', object='%s', ifc='%s', signal='%s'\n",
		sender_name, object_path, interface, signal_name);

	if (user_data != nullptr) {
		static_cast<DBusBluez*>(user_data)->Timestamp(object_path);
	}
}

static void cb_ifc_add(
	GDBusConnection *, // connection
	const gchar *,     // sender name
	const gchar *,     // object path
	const gchar *,     // interface name
	const gchar *,     // signal name
	GVariant *params,
	gpointer user_data)
{
	const char *object;

	GVariantIter *interfaces;

	//check_signature(params,"(&oa{sa{sv}})",signal_name);

	g_variant_get(params, "(&oa{sa{sv}})", &object, &interfaces);
	printf("Added [%s]:\n", object);

	if (user_data != nullptr) {
		static_cast<DBusBluez*>(user_data)->Timestamp(object);
	}

	/*
	{
	GVariant *properties;
	const gchar *interface_name;
	while(g_variant_iter_next(interfaces, "{&s@a{sv}}", &interface_name, &properties)) {
		printf("\t{%s}\n", interface_name);

		// old g_ascii_strdown modified in place, now returns new string - free it!
		auto lowered = g_ascii_strdown(interface_name, -1);

		// If there's a device on this interface, print data
		if(g_strstr_len(lowered, -1, "device") != nullptr) {
			GVariant *property_val;
			const gchar *property_name;
			GVariantIter i;

			g_variant_iter_init(&i, properties);
			while(g_variant_iter_next(&i, "{&sv}", &property_name, &property_val)) {
				gvariant_print("\t\t", property_name, property_val);
				g_variant_unref(property_val);
			}
		}

		g_free(lowered);
		g_variant_unref(properties);
	}
	printf("\n");
	}
	*/

	if (interfaces != nullptr) g_variant_iter_free(interfaces);
}

static void cb_ifc_rem(
	GDBusConnection *, // connection
	const gchar *,     // sender name
	const gchar *,     // object path
	const gchar *,     // interface name
	const gchar *,     // signal name
	GVariant *params,
	gpointer user_data)
{
	const char *object;

	GVariantIter *interfaces;

	//check_signature(params,"(&oas)",signal_name);

	g_variant_get(params, "(&oas)", &object, &interfaces);
	printf("Removed [%s]:\n", object);
	
	if (user_data != nullptr) {
		static_cast<DBusBluez*>(user_data)->Timestamp(object);
	}

	/*
	{
	const gchar *interface_name;
	while(g_variant_iter_next(interfaces, "s", &interface_name)) {
		printf("\t{%s}\n", interface_name);

		// old g_ascii_strdown modified in place, now returns new string - free it!
		auto lowered = g_ascii_strdown(interface_name, -1);

		// If there's a device on this interface, update data
		if (g_strstr_len(lowered, -1, "device") != nullptr) {
		}

		g_free(lowered);
	}
	printf("\n");
	}
	*/

	if (interfaces != nullptr) g_variant_iter_free(interfaces);
}

static void cb_prop(
	GDBusConnection *, // connection
	const gchar *,     // sender name
	const gchar *object_path,
	const gchar *,     // interface name
	const gchar *,     // signal name
	GVariant *params,
	gpointer user_data)
{
	GVariantIter *properties = nullptr;
	GVariant *value = nullptr;

	const char *iface;

	//check_signature(params,"(sa{sv}as)",signal_name);

	g_variant_get(params, "(&sa{sv}as)", &iface, &properties, nullptr);
	printf("Property [%s] {%s}:\n", object_path, iface);

	if (user_data != nullptr) {
		static_cast<DBusBluez*>(user_data)->Timestamp(object_path);
	}

//	/*
	{
	const char *key;
	while(g_variant_iter_next(properties, "{&sv}", &key, &value)) {
		gvariant_print("\t", key, value);
	}
	printf("\n");
	}
//	*/

	if (properties != nullptr) g_variant_iter_free(properties);
	if (value != nullptr) g_variant_unref(value);
}


DBusBluez::DBusBluez()
{
	loop = g_main_loop_new(nullptr, FALSE);
}

DBusBluez::~DBusBluez()
{
	g_main_loop_unref(loop);
}

void DBusBluez::Timestamp(const gchar *obj_id)
{
	const std::lock_guard<std::mutex> lock(mutex);
	devices[obj_id] = Clock::now();
}

// https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/adapter-api.txt
bool DBusBluez::SetPowered(bool is_on)
{
	GVariant *result;
	GError *error = nullptr;

	const gchar *bus_name = "org.bluez";
	const gchar *obj_path = "/org/bluez/hci0";

	const gchar *interf_name = "org.freedesktop.DBus.Properties";
	const gchar *method_name = "Set";
	auto parameters = g_variant_new("(ssv)", "org.bluez.Adapter1", "Powered", g_variant_new("b", is_on));

	// Floating reference to "parameters" etc is "sinked" in g_dbus_connection_call_sync() below.
	// Hence, don't need to call g_variant_unref() etc to prevent leaks.

	result = g_dbus_connection_call_sync(con,
		bus_name, obj_path,
		interf_name, method_name, parameters,
		nullptr,
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		nullptr,
		&error);

	if (error != nullptr) {
		auto str = is_on ? "on" : "off";
		printf("Power %s BT: %s\n", str, error->message);
		return false;
	}

	g_variant_unref(result);

	return true;
}

// https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/adapter-api.txt
bool DBusBluez::SetDiscovery(bool is_on)
{
	GVariant *result;
	GError *error = nullptr;

	const gchar *bus_name = "org.bluez";
	const gchar *obj_path = "/org/bluez/hci0";

	const gchar *interf_name = "org.bluez.Adapter1";
	const gchar *method_name = is_on ? "StartDiscovery" : "StopDiscovery";
	GVariant *parameters = nullptr;

	result = g_dbus_connection_call_sync(con,
		bus_name, obj_path,
		interf_name, method_name, parameters,
		nullptr,
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		nullptr,
		&error);
	
	if (error != nullptr) {
		auto str = is_on ? "on" : "off";
		printf("Turn %s BT discovery: %s\n", str, error->message);
		return false;
	}

	g_variant_unref(result);

	return true;
}

guint DBusBluez::RegisterCallback(
	const gchar *interf_name,
	const gchar *signal_name,
	SignalCallback callback)
{
	const gchar *sender_name = "org.bluez";

	auto cb = g_dbus_connection_signal_subscribe(con,
		sender_name, interf_name, signal_name,
		nullptr,
		nullptr,
		G_DBUS_SIGNAL_FLAGS_NONE,
		callback,
		this, nullptr); // user data = this pointer
	callbacks.push_back(cb);
	return cb;
}

void DBusBluez::Go( Flag monitor )
{
	if (g_main_loop_is_running(loop) == TRUE) {
		printf("Go(): main loop already running!\n");
		return;
	}

	con = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);
	if (con == nullptr) {
		printf("Go(): Not able to get connection to system bus\n");
		return;
	}

	if (monitor == Flags::None) {
		printf("Go(): no monitor flags; defaulting to firehose\n");
		monitor = Flags::Firehose;
	}

	// Firehose callback - gets *everything* from Bluez
	if (monitor == Flags::Firehose)
	{
		RegisterCallback(nullptr, nullptr, cb_firehose);
	}
	else
	{
		// Callbacks for properties interface
		if (monitor & Flags::PropertyChange)
		{
			const gchar *ifc_name = "org.freedesktop.DBus.Properties";
			RegisterCallback(ifc_name, "PropertiesChanged", cb_prop);
		}

		// Callbacks for object manager interface
		if (monitor & Flags::ObjectManager)
		{
			const gchar *ifc_name = "org.freedesktop.DBus.ObjectManager";
			RegisterCallback(ifc_name, "InterfacesAdded", cb_ifc_add);
			RegisterCallback(ifc_name, "InterfacesRemoved", cb_ifc_rem);
		}
	}

	if (!SetPowered(true)) goto cleanup;
	if (!SetDiscovery(true)) goto cleanup;

	g_main_loop_run(loop);

cleanup:

	if (!SetDiscovery(false)) printf("Go(): Discovery off failed\n");
	if (!SetPowered(false)) printf("Go(): Power off failed\n");

	printf("Go(): Removing %d callbacks ...\n", (int)callbacks.size());
	for (auto &cb : callbacks) g_dbus_connection_signal_unsubscribe(con, cb);
	
	printf("Go(): Dereferencing DBus connection ...\n");
	g_object_unref(con);
	
	callbacks.clear();
}
