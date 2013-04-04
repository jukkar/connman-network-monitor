/*
 *
 *  Network Monitor for Connection Manager
 *
 *  Copyright (C) 2012  Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License version 2.1,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <errno.h>

#include <gio/gio.h>

#include "connman-api.h"

#define CONNMAN_DBUS_NAME "net.connman"
#define CONNMAN_ERROR CONNMAN_DBUS_NAME ".Error"

#define CONNMAN_MANAGER_PATH "/"
#define CONNMAN_MANAGER_INTERFACE CONNMAN_DBUS_NAME ".Manager"

struct connman_manager {
	GDBusConnection *connection;
	guint connman_watch;
	guint property_changed_watch;
	connman_property_changed_cb property_changed_cb;
	void *property_user_data;
	gboolean connman_running;
	GCancellable *pending;
};

static gboolean update_property(struct connman_manager *manager,
				const char *property,
				void *value, void *user_data)
{
	DBG("property %s", property);

	if (manager == NULL || manager->property_changed_cb == NULL)
		return FALSE;

	manager->property_changed_cb(property, value, user_data);

	return TRUE;
}

static void get_properties_callback(GObject *source_object,
				GAsyncResult *res,
				gpointer user_data)
{
	struct connman_manager *manager = user_data;
	GVariant *value, *array, *dictionary;
	GVariantIter iter, tuple;
	GDBusMessage *reply;
	GError *error = NULL;
	const char *key;

	DBG("");

	reply = g_dbus_connection_send_message_with_reply_finish(
					manager->connection, res, &error);
	if (reply == NULL)
		goto error;

	if (error != NULL || g_dbus_message_to_gerror(reply, &error) == TRUE) {
		DBG("%s", error->message);
		g_error_free(error);
		goto error;
	}

	array = g_dbus_message_get_body(reply);
	g_variant_iter_init(&tuple, array);

	dictionary = g_variant_iter_next_value(&tuple);
	g_variant_iter_init (&iter, dictionary);

	while (g_variant_iter_loop(&iter, "{sv}", &key, &value)) {
		if (g_str_equal(key, "State") == TRUE) {
			const char *state;
			gsize len;

			state = g_variant_get_string(value, &len);

			update_property(manager, "State", (void *)state,
					manager->property_user_data);
		}
	}

error:
	if (reply != NULL)
		g_object_unref(reply);

	g_object_unref(manager->pending);
	manager->pending = NULL;
}

static void property_changed_signal_cb(GDBusConnection *connection,
					const gchar *sender_name,
					const gchar *object_path,
					const gchar *interface_name,
					const gchar *signal_name,
					GVariant *parameters,
					gpointer user_data)
{
	struct connman_manager *manager = user_data;
	GVariant *value;
	char *key;

	g_variant_get(parameters, "(sv)", &key, &value);

	if (g_str_equal(key, "State") == TRUE) {
		const char *state;
		gsize len;

		state = g_variant_get_string(value, &len);
		update_property(manager, "State", (void *)state,
				manager->property_user_data);
	}

	g_free(key);
}

static void connman_started(GDBusConnection *conn, const gchar *name,
			const gchar *name_owner, void *user_data)
{
	struct connman_manager *manager = user_data;

	DBG("connection %p manager %p", conn, manager);

	manager->connman_running = TRUE;
}

static void connman_stopped(GDBusConnection *conn, const gchar *name,
			void *user_data)
{
	struct connman_manager *manager = user_data;

	DBG("connection %p manager %p", conn, manager);

	manager->connman_running = FALSE;

	update_property(manager, "State", "idle", manager->property_user_data);
}

struct connman_manager *
connman_manager_init(connman_property_changed_cb property_changed_cb,
			void *user_data)
{
	struct connman_manager *manager;
	GDBusMessage *message;
	int err = 0;

	DBG("");

	manager = g_try_malloc0(sizeof(struct connman_manager));
	if (manager == NULL)
		return NULL;

	manager->connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
	manager->property_changed_cb = property_changed_cb;
	manager->property_user_data = user_data;

	message = g_dbus_message_new_method_call(CONNMAN_DBUS_NAME,
						CONNMAN_MANAGER_PATH,
						CONNMAN_MANAGER_INTERFACE,
						"GetProperties");
	if (message == NULL) {
		err = -ENOMEM;
		goto error;
	}

	manager->pending = g_cancellable_new();
	g_dbus_connection_send_message_with_reply(manager->connection,
						message,
						G_DBUS_SEND_MESSAGE_FLAGS_NONE,
						-1,
						NULL,
						manager->pending,
						get_properties_callback,
						manager);

	manager->connman_watch = g_bus_watch_name(G_BUS_TYPE_SYSTEM,
						CONNMAN_DBUS_NAME,
						G_BUS_NAME_WATCHER_FLAGS_NONE,
						connman_started,
						connman_stopped,
						manager,
						NULL);
	if (manager->connman_watch == 0) {
		err = -EINVAL;
		goto error;
	}

	manager->property_changed_watch =
		g_dbus_connection_signal_subscribe(manager->connection,
						CONNMAN_DBUS_NAME,
						CONNMAN_MANAGER_INTERFACE,
						"PropertyChanged",
						CONNMAN_MANAGER_PATH,
						NULL,
						G_DBUS_SIGNAL_FLAGS_NONE,
						property_changed_signal_cb,
						manager,
						NULL);

	if (manager->property_changed_watch == 0) {
		err = -EINVAL;
		goto error;
	}

error:
	if (message != NULL)
		g_object_unref(message);

	if (err < 0) {
		DBG("Cannot initialize manager");
		connman_manager_cleanup(manager);
		return NULL;
	}

	return manager;
}

void connman_manager_cleanup(struct connman_manager *manager)
{
	if (manager == NULL)
		return;

	DBG("");

	manager->property_changed_cb = NULL;
	manager->connman_running = FALSE;

	if (manager->pending != NULL)
		g_cancellable_cancel(manager->pending);

	if (manager->property_changed_watch != 0) {
		g_dbus_connection_signal_unsubscribe(manager->connection,
					manager->property_changed_watch);
		manager->property_changed_watch = 0;
	}

	if (manager->connman_watch != 0) {
		g_bus_unwatch_name(manager->connman_watch);
		manager->connman_watch = 0;
	}

	g_object_unref(manager->connection);

	manager->property_user_data = NULL;

	g_free(manager);
	manager = NULL;
}
