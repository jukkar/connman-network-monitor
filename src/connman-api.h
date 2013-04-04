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

typedef void (*connman_property_changed_cb)(const char *property,
					void *value,
					void *user_data);

struct connman_manager;

struct connman_manager *
connman_manager_init(connman_property_changed_cb property_changed_cb,
			void *user_data);

void connman_manager_cleanup(struct connman_manager *manager);

#define DBG(fmt, arg...) do {					       \
	g_debug("%s:%s() " fmt "\n", __FILE__, __FUNCTION__ , ## arg); \
} while (0)
