/*
 * GTK VNC Widget
 *  
 * Copyright (C) 2006  Anthony Liguori <anthony@codemonkey.ws>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#define DEBUG_GTK_VNC

#include "vmrunner.h"

#ifdef DEBUG_GTK_VNC
#define DPRINTF(fmt, ...) \
do { char *dtmp = get_datetime(); fprintf(stderr, "[%s ", dtmp); free(dtmp); dtmp=NULL; fprintf(stderr, "pvr/gtk-vnc    ] " fmt , ## __VA_ARGS__); fflush(stderr); } while (0)
#else
#define DPRINTF(fmt, ...) \
do {} while(0)
#endif

#ifndef HAVE_LIBGTK_VNC_1_0
int startVNCViewer(char *hostname, char *port, int fullscreen)
{
	/* Always return error of no connection available for case we don't have gtk-vnc support compiled */
	return VNC_STATUS_NO_CONNECTION;
}
#else
/* We have gtk-vnc support */
#if WITH_LIBVIEW
#include <libview/autoDrawer.h>
#endif

static const GOptionEntry options [] =
{
  { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, 0 }
};

static GtkWidget *vnc, *window;
static gboolean connected;
static gboolean modifier_super_l = FALSE;
int retCode = VNC_STATUS_OK;

static void set_title(VncDisplay *vncdisplay, GtkWidget *window,
	gboolean grabbed)
{
	const char *name;
	char title[1024] = { 0 };
	const char *subtitle;

	if (grabbed)
		subtitle = "(Press Ctrl+Alt to release pointer) ";
	else
		subtitle = "";

	name = vnc_display_get_name(VNC_DISPLAY(vncdisplay));
	snprintf(title, sizeof(title), "%s%s",
		 subtitle, name);

	gtk_window_set_title(GTK_WINDOW(window), title);
}

static gboolean is_fullscreen (GtkWidget *window G_GNUC_UNUSED)
{
	return ((gdk_window_get_state(GTK_WIDGET(window)->window) & GDK_WINDOW_STATE_FULLSCREEN) != 0);
}

static gboolean handle_key_press(GtkWidget *window G_GNUC_UNUSED,
	GdkEvent *ev, GtkWidget *vncdisplay)
{
        if ((ev->key.keyval == GDK_Super_L) && (ev->key.is_modifier))
		modifier_super_l = TRUE;
	if (((ev->key.keyval == GDK_F) || (ev->key.keyval == GDK_f))
             && (modifier_super_l)) {
		if (!is_fullscreen(window))
			gtk_window_fullscreen(GTK_WINDOW(window));
		else
			gtk_window_unfullscreen(GTK_WINDOW(window));

		return TRUE;
	}
	return FALSE;
}

static gboolean handle_key_release(GtkWidget *window G_GNUC_UNUSED,
        GdkEvent *ev, GtkWidget *vncdisplay)
{
	if ((ev->key.keyval == GDK_Super_L) && (ev->key.is_modifier))
		modifier_super_l = FALSE;
        return FALSE;
}

static void vnc_grab(GtkWidget *vncdisplay, GtkWidget *window)
{
	set_title(VNC_DISPLAY(vncdisplay), window, TRUE);
}

static void vnc_ungrab(GtkWidget *vncdisplay, GtkWidget *window)
{
	set_title(VNC_DISPLAY(vncdisplay), window, FALSE);
}

static void vnc_connected(GtkWidget *vncdisplay G_GNUC_UNUSED)
{
	DPRINTF("Connected to server\n");
	connected = TRUE;
}

static void vnc_initialized(GtkWidget *vncdisplay, GtkWidget *window)
{
	DPRINTF("Connection initialized to: %s\n",
		vnc_display_get_name(VNC_DISPLAY(vncdisplay)));
	set_title(VNC_DISPLAY(vncdisplay), window, FALSE);
	gtk_widget_show_all(window);
}

static void vnc_auth_failure(GtkWidget *vncdisplay G_GNUC_UNUSED,
	const char *msg)
{
	DPRINTF("Authentication failed '%s'\n", msg ? msg : "");
}

static void vnc_desktop_resize(GtkWidget *vncdisplay G_GNUC_UNUSED,
	int width, int height)
{
	DPRINTF("Remote desktop size changed to %dx%d\n", width, height);
}

static void vnc_disconnected(GtkWidget *vncdisplay G_GNUC_UNUSED)
{
        if (!connected) {
		DPRINTF("Cannot connect to server\n");
		retCode = VNC_STATUS_NO_CONNECTION;
	}
	else {
		DPRINTF("Disconnected from server\n");
		retCode = VNC_STATUS_SHUTDOWN;
	}

	//gtk_widget_destroy(vnc);
	//gtk_widget_destroy(window);
	//window = NULL;
	//vnc = NULL;
	gtk_widget_hide(window);

	gtk_main_quit();
}

static void vnc_credential(GtkWidget *vncdisplay, GValueArray *credList)
{
	GtkWidget *dialog = NULL;
	int response;
	unsigned int i, prompt = 0;
	const char **data;

	DPRINTF("Got credential request for %d credential(s)\n", credList->n_values);

	data = g_new0(const char *, credList->n_values);

	for (i = 0 ; i < credList->n_values ; i++) {
		GValue *cred = g_value_array_get_nth(credList, i);
		switch (g_value_get_enum(cred)) {
		case VNC_DISPLAY_CREDENTIAL_USERNAME:
		case VNC_DISPLAY_CREDENTIAL_PASSWORD:
			prompt++;
			break;
		case VNC_DISPLAY_CREDENTIAL_CLIENTNAME:
			data[i] = "fvncviewer";
		default:
			break;
		}
	}

	if (prompt) {
		GtkWidget **label, **entry, *box, *vbox;
		int row;
		dialog = gtk_dialog_new_with_buttons("Authentication required",
						     NULL,
						     0,
						     GTK_STOCK_CANCEL,
						     GTK_RESPONSE_CANCEL,
						     GTK_STOCK_OK,
						     GTK_RESPONSE_OK,
						     NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

		box = gtk_table_new(credList->n_values, 2, FALSE);
		label = g_new(GtkWidget *, prompt);
		entry = g_new(GtkWidget *, prompt);

		for (i = 0, row =0 ; i < credList->n_values ; i++) {
			GValue *cred = g_value_array_get_nth(credList, i);
			entry[row] = gtk_entry_new();
			switch (g_value_get_enum(cred)) {
			case VNC_DISPLAY_CREDENTIAL_USERNAME:
				label[row] = gtk_label_new("Username:");
				break;
			case VNC_DISPLAY_CREDENTIAL_PASSWORD:
				label[row] = gtk_label_new("Password:");
				gtk_entry_set_activates_default(GTK_ENTRY(entry[row]), TRUE);
				break;
			default:
				continue;
			}
			if (g_value_get_enum (cred) == VNC_DISPLAY_CREDENTIAL_PASSWORD)
				gtk_entry_set_visibility (GTK_ENTRY (entry[row]), FALSE);

			gtk_table_attach(GTK_TABLE(box), label[i], 0, 1, row, row+1, GTK_SHRINK, GTK_SHRINK, 3, 3);
			gtk_table_attach(GTK_TABLE(box), entry[i], 1, 2, row, row+1, GTK_SHRINK, GTK_SHRINK, 3, 3);
			row++;
		}

		vbox = gtk_bin_get_child(GTK_BIN(dialog));
		gtk_container_add(GTK_CONTAINER(vbox), box);

		gtk_widget_show_all(dialog);
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_hide(GTK_WIDGET(dialog));

		if (response == GTK_RESPONSE_OK) {
			for (i = 0, row = 0 ; i < credList->n_values ; i++) {
				GValue *cred = g_value_array_get_nth(credList, i);
				switch (g_value_get_enum(cred)) {
				case VNC_DISPLAY_CREDENTIAL_USERNAME:
				case VNC_DISPLAY_CREDENTIAL_PASSWORD:
					data[i] = gtk_entry_get_text(GTK_ENTRY(entry[row]));
					break;
				default:
					continue;
				}
				row++;
			}
		}
	}

	for (i = 0 ; i < credList->n_values ; i++) {
		GValue *cred = g_value_array_get_nth(credList, i);
		if (data[i]) {
			if (vnc_display_set_credential(VNC_DISPLAY(vncdisplay),
						       g_value_get_enum(cred),
						       data[i])) {
				DPRINTF("Failed to set credential type %d\n", g_value_get_enum(cred));
				vnc_display_close(VNC_DISPLAY(vncdisplay));
			}
		} else {
			DPRINTF("Unsupported credential type %d\n", g_value_get_enum(cred));
			vnc_display_close(VNC_DISPLAY(vncdisplay));
		}
	}

	g_free(data);
	if (dialog)
		gtk_widget_destroy(GTK_WIDGET(dialog));
}

#if WITH_LIBVIEW
static gboolean window_state_event(GtkWidget *widget,
				   GdkEventWindowState *event,
				   gpointer data)
{
	ViewAutoDrawer *drawer = VIEW_AUTODRAWER(data);

	if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) {
		if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) {
			vnc_display_force_grab(VNC_DISPLAY(vnc), TRUE);
			ViewAutoDrawer_SetActive(drawer, TRUE);
		} else {
			vnc_display_force_grab(VNC_DISPLAY(vnc), FALSE);
			ViewAutoDrawer_SetActive(drawer, FALSE);
		}
	}

	return FALSE;
}
#endif

int startVNCViewer(char *hostname, char *port, int fullscreen)
{
	GOptionContext *context;
	GError *error = NULL;
	GtkWidget *layout;

	DPRINTF("Starting VNC viewer for host %s and port %s\n", hostname, port);

	context = g_option_context_new ("- VMRunner GTK-VNC client");
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_add_group (context, vnc_display_get_option_group ());
	g_option_context_parse (context, 0, NULL, &error);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	vnc = vnc_display_new();
#if WITH_LIBVIEW
	layout = ViewAutoDrawer_New();
#else
	layout = gtk_vbox_new(FALSE, 0);
#endif

	if (fullscreen)
	        gtk_window_fullscreen(GTK_WINDOW(window));

#if WITH_LIBVIEW
	ViewAutoDrawer_SetActive(VIEW_AUTODRAWER(layout), FALSE);
	ViewOvBox_SetUnder(VIEW_OV_BOX(layout), vnc);
#else
	gtk_box_pack_start(GTK_BOX(layout), vnc, TRUE, TRUE, 0);
#endif
	gtk_widget_show(window);
	gtk_container_add(GTK_CONTAINER(window), layout);

	gtk_widget_realize(vnc);

	DPRINTF("Opening VNC host connection\n");
	vnc_display_open_host(VNC_DISPLAY(vnc), hostname, port);
	vnc_display_set_keyboard_grab(VNC_DISPLAY(vnc), TRUE);
	vnc_display_set_pointer_grab(VNC_DISPLAY(vnc), TRUE);

	if (!gtk_widget_is_composited(window)) {
		vnc_display_set_scaling(VNC_DISPLAY(vnc), TRUE);
	}

	gtk_signal_connect(GTK_OBJECT(window), "delete-event",
			   GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
	gtk_signal_connect(GTK_OBJECT(vnc), "vnc-connected",
			   GTK_SIGNAL_FUNC(vnc_connected), NULL);
	gtk_signal_connect(GTK_OBJECT(vnc), "vnc-initialized",
			   GTK_SIGNAL_FUNC(vnc_initialized), window);
	gtk_signal_connect(GTK_OBJECT(vnc), "vnc-disconnected",
			   GTK_SIGNAL_FUNC(vnc_disconnected), NULL);
	gtk_signal_connect(GTK_OBJECT(vnc), "vnc-auth-credential",
			   GTK_SIGNAL_FUNC(vnc_credential), NULL);
	gtk_signal_connect(GTK_OBJECT(vnc), "vnc-auth-failure",
			   GTK_SIGNAL_FUNC(vnc_auth_failure), NULL);

	gtk_signal_connect(GTK_OBJECT(vnc), "vnc-desktop-resize",
			   GTK_SIGNAL_FUNC(vnc_desktop_resize), NULL);

	gtk_signal_connect(GTK_OBJECT(vnc), "vnc-pointer-grab",
			   GTK_SIGNAL_FUNC(vnc_grab), window);
	gtk_signal_connect(GTK_OBJECT(vnc), "vnc-pointer-ungrab",
			   GTK_SIGNAL_FUNC(vnc_ungrab), window);

	gtk_signal_connect(GTK_OBJECT(window), "key-press-event",
			   GTK_SIGNAL_FUNC(handle_key_press), vnc);
	gtk_signal_connect(GTK_OBJECT(window), "key-release-event",
			   GTK_SIGNAL_FUNC(handle_key_release), vnc);

#if WITH_LIBVIEW
	gtk_signal_connect(GTK_OBJECT(window), "window-state-event",
			   GTK_SIGNAL_FUNC(window_state_event), layout);
#endif

	gtk_main();

	DPRINTF("Done. Viewer has finished with code %d\n", retCode);
	return retCode;
}
#endif

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
