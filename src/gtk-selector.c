/*
 * GTK Selector for PVR
 *  
 * Copyright (C) 2012  Michal Novotny <mignov@gmail.com>
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

#define DEBUG_GTK_SELECTOR

#include "vmrunner.h"

#ifdef DEBUG_GTK_SELECTOR
#define DPRINTF(fmt, ...) \
do { char *dtmp = get_datetime(); fprintf(stderr, "[%s ", dtmp); free(dtmp); dtmp=NULL; fprintf(stderr, "pvr/gtk-select ] " fmt , ## __VA_ARGS__); fflush(stderr); } while (0)
#else
#define DPRINTF(fmt, ...) \
do {} while(0)
#endif

#ifndef HAVE_LIBGTK_VNC_1_0
int show_selector(char *title, char **items, int num_items)
{
	/* Always return error if no GTK-VNC library found */
	return -1;
}
#else
/* We have gtk-vnc support */

int g_select_global_id;
GtkWidget *gswindow;

void get_selection_id(GtkWidget	*gtklist, gpointer func_data)
{
	GList *dlist = GTK_LIST(gtklist)->selection;
	if (!dlist)
		return;

	g_select_global_id = -1;
	while (dlist) {
		GtkObject	*list_item;
		gchar		*item_data_string;
        
		list_item=GTK_OBJECT(dlist->data);
		item_data_string=gtk_object_get_data(list_item,
				"list-selector");

		if (item_data_string != NULL) {
			gchar **tmp = g_strsplit(item_data_string, ")", 2);

			g_select_global_id = atoi(tmp[0]);

			DPRINTF("Option %d (%s) has been selected\n", g_select_global_id, (char *)tmp[1]);
			g_strfreev(tmp);

		}

		dlist = dlist->next;
	}

	gtk_widget_hide(gswindow);
	//gtk_widget_destroy(gtklist);
	gtk_main_quit();
}

int show_selector(char *title, char **items, int num_items)
{
	GtkWidget *vbox;
	GtkWidget *gtklist;
	GtkWidget *list_item;
	guint i;
	gchar buffer[64];

	if ((title == NULL) || (items == NULL) || (num_items == 0))
		return 0;

	gtk_init(NULL, NULL);
 
	gswindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(gswindow), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_default_size(GTK_WINDOW(gswindow), 400, 300);
	gtk_window_set_title(GTK_WINDOW(gswindow), title);
	gtk_signal_connect(GTK_OBJECT(gswindow),
			"destroy",
			GTK_SIGNAL_FUNC(gtk_main_quit),
			NULL);
    
    
	vbox=gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add(GTK_CONTAINER(gswindow), vbox);
	gtk_widget_show(vbox);
    
	gtklist=gtk_list_new();
	gtk_container_add(GTK_CONTAINER(vbox), gtklist);
	gtk_widget_show(gtklist);
	gtk_signal_connect(GTK_OBJECT(gtklist),
			"selection_changed",
			GTK_SIGNAL_FUNC(get_selection_id),
			NULL);
   
	for (i = 0; i < num_items; i++) {
		GtkWidget	*label;
		gchar		*string;
        
		sprintf(buffer, "%d) %s", i, items[i]);
		label=gtk_label_new(buffer);
		list_item=gtk_list_item_new();
		gtk_container_add(GTK_CONTAINER(list_item), label);
		gtk_widget_show(label);
		gtk_container_add(GTK_CONTAINER(gtklist), list_item);
		gtk_widget_show(list_item);
		gtk_label_get(GTK_LABEL(label), &string);
		gtk_object_set_data(GTK_OBJECT(list_item),
			"list-selector",
			string);
	}

	gtk_widget_show(gswindow);
	gtk_main();

	while (gtk_events_pending ())
		gtk_main_iteration ();
 
	DPRINTF("Selector is returning %d\n", g_select_global_id);   
	return g_select_global_id;
}
#endif

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
