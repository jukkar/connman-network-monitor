/*
 *
 *  Network Monitor for Connection Manager
 *
 *  Copyright (C) 2012  Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdlib.h>
#include <signal.h>
#include <sys/signalfd.h>

#include <gio/gio.h>

static unsigned int __terminated = 0;
static GMainLoop *loop;

static gboolean check_host(gpointer data)
{
	GNetworkMonitor *monitor;
	GSocketConnectable *addr;
	GCancellable *cancellable;
	GError *error = NULL;
	const char *host = data;
	gboolean reachable;

	monitor =  g_network_monitor_get_default();

	addr = g_network_address_new(host, 80);

	cancellable = g_cancellable_new();

	reachable = g_network_monitor_can_reach(monitor,
						addr,
						cancellable,
						&error);

	g_print("Host %s is %sreachable\n", host, reachable ? "" : "not ");
	if (error != NULL) {
		g_print("Error: %s (%d)\n", error->message, error->code);
		g_error_free(error);
	}

	return FALSE;
}

static void
watch_network_changed(GNetworkMonitor * monitor,
		      gboolean available, gpointer user_data)
{
	g_print("Network is %s\n", available ? "up" : "down");

	g_timeout_add_seconds(0, check_host, user_data);
}

static gboolean signal_handler(GIOChannel *channel, GIOCondition cond,
							gpointer user_data)
{
	struct signalfd_siginfo si;
	ssize_t result;
	int fd;

	if (cond & (G_IO_NVAL | G_IO_ERR | G_IO_HUP))
		return FALSE;

	fd = g_io_channel_unix_get_fd(channel);

	result = read(fd, &si, sizeof(si));
	if (result != sizeof(si))
		return FALSE;

	switch (si.ssi_signo) {
	case SIGINT:
	case SIGTERM:
		if (__terminated == 0) {
			g_print("Terminating");
			g_main_loop_quit(loop);
		}

		__terminated = 1;
		break;
	}

	return TRUE;
}

static guint setup_signalfd(void)
{
	GIOChannel *channel;
	guint source;
	sigset_t mask;
	int fd;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
		perror("Failed to set signal mask");
		return 0;
	}

	fd = signalfd(-1, &mask, 0);
	if (fd < 0) {
		perror("Failed to create signal descriptor");
		return 0;
	}

	channel = g_io_channel_unix_new(fd);

	g_io_channel_set_close_on_unref(channel, TRUE);
	g_io_channel_set_encoding(channel, NULL, NULL);
	g_io_channel_set_buffered(channel, FALSE);

	source = g_io_add_watch(channel,
				G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
				signal_handler, NULL);

	g_io_channel_unref(channel);

	return source;
}

int main(int argc, char **argv)
{
	GNetworkMonitor *monitor;
	gboolean available;
	const char *host = "www.connman.net";
	guint signal;

	g_type_init();

	if (argc > 1 && strcmp(argv[1], "-h") == 0) {
		g_print("Usage: %s [<hostname>]\n", argv[0]);
		exit(-1);
	} else if (argc > 1)
		host = argv[1];

	monitor =  g_network_monitor_get_default();

	g_print("Monitoring via %s\n",
		g_type_name_from_instance((GTypeInstance *) monitor));

	g_signal_connect(monitor, "network-changed",
			G_CALLBACK(watch_network_changed), (gpointer)host);

	available = g_network_monitor_get_network_available(monitor);
	g_print("Initial network availibility is %s\n",
		available ? "yes" : "no");

	signal = setup_signalfd();

	loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	g_source_remove(signal);

	g_object_unref(monitor);
}
