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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gio/gio.h>

#include "connman-api.h"

static int priority = 90;
static guint network_changed_signal = 0;

enum {
	PROP_0,
	PROP_NETWORK_AVAILABLE,
	PROP_CONNECTIVITY,
	PROP_METERED,
};

enum connman_state {
	STATE_UNKNOWN = 0,
	STATE_IDLE,
	STATE_READY,
	STATE_ONLINE,
	STATE_FAILURE,
};

typedef struct _GNetworkMonitorConnmanPrivate GNetworkMonitorConnmanPrivate;
struct _GNetworkMonitorConnmanPrivate
{
	enum connman_state state;
	struct connman_manager *manager;
};

typedef struct _GNetworkMonitorConnman GNetworkMonitorConnman;
struct _GNetworkMonitorConnman {
	GObject parent;
};

typedef struct _GNetworkMonitorConnmanClass GNetworkMonitorConnmanClass;
struct _GNetworkMonitorConnmanClass {
	GObjectClass parent_class;
};

#define CONNMAN_TYPE_NETWORK_MONITOR (g_network_monitor_connman_get_type())
#define CONNMAN_NETWORK_MONITOR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),	\
					CONNMAN_TYPE_NETWORK_MONITOR,   \
					GNetworkMonitorConnman))
#define CONNMAN_NETWORK_MONITOR_CLASS(klass) \
					(G_TYPE_CHECK_CLASS_CAST((klass), \
						CONNMAN_TYPE_NETWORK_MONITOR, \
						GNetworkMonitorConnmanClass))
#define CONNMAN_IS_NETWORK_MONITOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
						CONNMAN_TYPE_NETWORK_MONITOR))
#define CONNMAN_IS_NETWORK_MONITOR_CLASS(klass) \
					(G_TYPE_CHECK_CLASS_TYPE((klass), \
						CONNMAN_TYPE_NETWORK_MONITOR))

static void network_monitor_iface_init(GNetworkMonitorInterface *iface);
static void network_monitor_initable_iface_init(GInitableIface *iface);
static void g_network_monitor_connman_init(GNetworkMonitorConnman *self);
static void g_network_monitor_connman_class_init(GNetworkMonitorConnmanClass *klass);
static void g_network_monitor_connman_class_finalize(GNetworkMonitorConnmanClass *klass);
GType g_network_monitor_connman_get_type(void);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(GNetworkMonitorConnman,
			g_network_monitor_connman,
			G_TYPE_OBJECT, 0  /* flags */,
			G_IMPLEMENT_INTERFACE_DYNAMIC(G_TYPE_INITABLE,
				network_monitor_initable_iface_init)
			G_IMPLEMENT_INTERFACE_DYNAMIC(G_TYPE_NETWORK_MONITOR,
				network_monitor_iface_init)
			G_ADD_PRIVATE_DYNAMIC(GNetworkMonitorConnman))

static GObject *network_monitor_constructor(GType type, guint n_props,
						GObjectConstructParam *props)
{
	GObject *obj;

	obj = G_OBJECT_CLASS(g_network_monitor_connman_parent_class)->
					constructor(type, n_props, props);
	return obj;
}

static void network_monitor_finalize(GObject *object)
{
	GNetworkMonitorConnman *monitor;
	GNetworkMonitorConnmanPrivate *priv;

	g_return_if_fail(object != NULL);
	g_return_if_fail(CONNMAN_IS_NETWORK_MONITOR(object));

	monitor = CONNMAN_NETWORK_MONITOR(object);

	priv = g_network_monitor_connman_get_instance_private(monitor);
	connman_manager_cleanup(priv->manager);
	priv->manager = NULL;
	priv->state = STATE_UNKNOWN;

	G_OBJECT_CLASS(g_network_monitor_connman_parent_class)->
					finalize(object);
}

static gboolean is_available(enum connman_state state)
{
	switch (state) {
	case STATE_UNKNOWN:
	case STATE_IDLE:
	case STATE_FAILURE:
		return FALSE;

	case STATE_READY:
	case STATE_ONLINE:
		return TRUE;
	}

	return FALSE;
}

static gboolean get_state(GNetworkMonitorConnman *monitor)
{
	GNetworkMonitorConnmanPrivate *priv = g_network_monitor_connman_get_instance_private(monitor);
	return is_available(priv->state);
}

static void get_property(GObject *object, guint prop_id,
			GValue *value, GParamSpec *pspec)
{
	GNetworkMonitorConnman *monitor = CONNMAN_NETWORK_MONITOR(object);

	switch (prop_id) {
	case PROP_NETWORK_AVAILABLE:
		g_value_set_boolean(value, get_state(monitor));
		break;

	case PROP_CONNECTIVITY:
		/* FIXME: Implement connectivity and captive portal checking. */
		g_value_set_enum(value, get_state(monitor) ? G_NETWORK_CONNECTIVITY_FULL : G_NETWORK_CONNECTIVITY_LOCAL);
		break;

	case PROP_METERED:
		/* FIXME: Implement metered checking. */
		g_value_set_boolean(value, FALSE);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void
g_network_monitor_connman_class_init(GNetworkMonitorConnmanClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->constructor = network_monitor_constructor;
	gobject_class->finalize = network_monitor_finalize;
	gobject_class->get_property = get_property;

	g_object_class_override_property(gobject_class,
					PROP_NETWORK_AVAILABLE,
					"network-available");
	g_object_class_override_property(gobject_class,
					PROP_CONNECTIVITY,
					"connectivity");
	g_object_class_override_property(gobject_class,
					PROP_METERED,
					"network-metered");
}

static void
g_network_monitor_connman_class_finalize(GNetworkMonitorConnmanClass *klass)
{
}

static enum connman_state string2state(const char *state)
{
	if (g_strcmp0(state, "idle") == 0)
		return STATE_IDLE;
	else if (g_strcmp0(state, "ready") == 0)
		return STATE_READY;
	else if (g_strcmp0(state, "online") == 0)
		return STATE_ONLINE;
	else if (g_strcmp0(state, "failure") == 0)
		return STATE_FAILURE;

	return STATE_UNKNOWN;
}

static void property_changed(const char *property, void *value,
							void *user_data)
{
	GNetworkMonitorConnman *monitor = user_data;
	GNetworkMonitorConnmanPrivate *priv;
	enum connman_state old_state, new_state;

	if (monitor == NULL) {
		DBG("monitor missing");
		return;
	}

	priv = g_network_monitor_connman_get_instance_private(monitor);
	old_state = new_state = priv->state;

	if (g_strcmp0(property, "State") == 0) {
		new_state = string2state(value);
		DBG("property %s value \"%s\"", property, (char *)value);
	}

	if (is_available(new_state) != is_available(old_state)) {
		priv->state = new_state;
		g_signal_emit(monitor, network_changed_signal, 0,
							get_state(monitor));
	}
}

static void g_network_monitor_connman_init(GNetworkMonitorConnman *self)
{
	/* Leak the module to keep it from being unloaded. */
	g_type_plugin_use (g_type_get_plugin (CONNMAN_TYPE_NETWORK_MONITOR));
}

static gboolean network_monitor_initable_init(GInitable *initable,
					GCancellable *cancellable,
					GError **error)
{
	GNetworkMonitorConnman *cm = CONNMAN_NETWORK_MONITOR(initable);
	GNetworkMonitorConnmanPrivate *priv = g_network_monitor_connman_get_instance_private(cm);

	priv->manager = connman_manager_init(property_changed, cm);

	DBG("cm %p manager %p", cm, priv->manager);

	return TRUE;
}

static void network_monitor_initable_iface_init(GInitableIface *iface)
{
	iface->init = network_monitor_initable_init;
}

static gboolean is_local(GSocketConnectable *connectable,
			GCancellable *cancellable)
{
	GSocketAddressEnumerator *enumerator;
	GSocketAddress *addr;
	GError *error = NULL;

	enumerator = g_socket_connectable_proxy_enumerate(connectable);
	addr = g_socket_address_enumerator_next(enumerator, cancellable,
						&error);
	if (addr == NULL) {
		g_object_unref(enumerator);
		return FALSE;
	}

	while (addr != NULL) {
		if (G_IS_INET_SOCKET_ADDRESS(addr)) {
			GInetAddress *iaddr;

			iaddr = g_inet_socket_address_get_address(
						G_INET_SOCKET_ADDRESS(addr));
			if (g_inet_address_get_is_loopback(iaddr) == TRUE) {
				g_object_unref(addr);
				g_object_unref(enumerator);
				return TRUE;
			}
		}

		g_object_unref(addr);
		addr = g_socket_address_enumerator_next(enumerator,
							cancellable,
							&error);
	}

	g_object_unref(enumerator);
	return FALSE;
}

static gboolean can_reach(GNetworkMonitor *monitor,
				GSocketConnectable *connectable,
				GCancellable *cancellable,
				GError **error)
{
	GNetworkMonitorConnman *cm = CONNMAN_NETWORK_MONITOR(monitor);

	DBG("");

	if (get_state(cm) == FALSE) {
		if (is_local(connectable, cancellable) == TRUE)
			return TRUE;

		if (error && *error == NULL)
			g_set_error_literal(error, G_IO_ERROR,
					G_IO_ERROR_NETWORK_UNREACHABLE,
					"No network connections available");
		return FALSE;
	}

	return TRUE;
}

static void network_monitor_iface_init(GNetworkMonitorInterface *iface)
{
	network_changed_signal = g_signal_lookup("network-changed",
						G_TYPE_NETWORK_MONITOR);

	iface->can_reach = can_reach;
}

void g_io_module_load(GIOModule *module)
{
	g_network_monitor_connman_register_type (G_TYPE_MODULE (module));
	g_io_extension_point_implement (G_NETWORK_MONITOR_EXTENSION_POINT_NAME,
	                                CONNMAN_TYPE_NETWORK_MONITOR,
	                                "connman",
	                                priority);
}

void g_io_module_unload(GIOModule *module)
{
}

char** g_io_module_query(void)
{
	char **eps = g_new(char *, 2);
	eps[0] = g_strdup(G_NETWORK_MONITOR_EXTENSION_POINT_NAME);
	eps[1] = NULL;
	return eps;
}
