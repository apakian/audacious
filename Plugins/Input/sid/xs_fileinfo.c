/*
   XMMS-SID - SIDPlay input plugin for X MultiMedia System (XMMS)

   File information window

   Programmed and designed by Matti 'ccr' Hamalainen <ccr@tnsp.org>
   (C) Copyright 1999-2005 Tecnic Software productions (TNSP)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "xs_fileinfo.h"
#include "xs_support.h"
#include "xs_stil.h"
#include "xs_config.h"
#include "xs_interface.h"
#include "xs_glade.h"

static GtkWidget *xs_fileinfowin = NULL;
static t_xs_stil_node *xs_fileinfostil = NULL;
XS_MUTEX(xs_fileinfowin);

#define LUW(x)	lookup_widget(xs_fileinfowin, x)


void xs_fileinfo_update(void)
{
	gboolean isEnabled;
	GtkAdjustment *tmpAdj;

	XS_MUTEX_LOCK(xs_status);
	XS_MUTEX_LOCK(xs_fileinfowin);

	/* Check if control window exists, we are currently playing and have a tune */
	if (xs_fileinfowin) {
		if (xs_status.tuneInfo && xs_status.isPlaying && (xs_status.tuneInfo->nsubTunes > 1)) {
			tmpAdj = gtk_range_get_adjustment(GTK_RANGE(LUW("fileinfo_subctrl_adj")));

			tmpAdj->value = xs_status.currSong;
			tmpAdj->lower = 1;
			tmpAdj->upper = xs_status.tuneInfo->nsubTunes;
			XS_MUTEX_UNLOCK(xs_status);
			XS_MUTEX_UNLOCK(xs_fileinfowin);
			gtk_adjustment_value_changed(tmpAdj);
			XS_MUTEX_LOCK(xs_status);
			XS_MUTEX_LOCK(xs_fileinfowin);
			isEnabled = TRUE;
		} else
			isEnabled = FALSE;

		/* Enable or disable subtune-control in fileinfo window */
		gtk_widget_set_sensitive(LUW("fileinfo_subctrl_prev"), isEnabled);
		gtk_widget_set_sensitive(LUW("fileinfo_subctrl_adj"), isEnabled);
		gtk_widget_set_sensitive(LUW("fileinfo_subctrl_next"), isEnabled);
	}

	XS_MUTEX_UNLOCK(xs_status);
	XS_MUTEX_UNLOCK(xs_fileinfowin);
}


void xs_fileinfo_setsong(void)
{
	gint n;

	XS_MUTEX_LOCK(xs_status);
	XS_MUTEX_LOCK(xs_fileinfowin);

	if (xs_status.tuneInfo && xs_status.isPlaying) {
		n = (gint) gtk_range_get_adjustment(GTK_RANGE(LUW("fileinfo_subctrl_adj")))->value;
		if ((n >= 1) && (n <= xs_status.tuneInfo->nsubTunes))
			xs_status.currSong = n;
	}

	XS_MUTEX_UNLOCK(xs_fileinfowin);
	XS_MUTEX_UNLOCK(xs_status);
}


void xs_fileinfo_ok(void)
{
	XS_MUTEX_LOCK(xs_fileinfowin);
	if (xs_fileinfowin) {
		gtk_widget_destroy(xs_fileinfowin);
		xs_fileinfowin = NULL;
	}
	XS_MUTEX_UNLOCK(xs_fileinfowin);
}


gboolean xs_fileinfo_delete(GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	(void) widget;
	(void) event;
	(void) user_data;

	XSDEBUG("delete_event\n");
	xs_fileinfo_ok();
	return FALSE;
}



void xs_fileinfo_subtune(GtkWidget * widget, void *data)
{
	t_xs_stil_subnode *tmpNode;
	GtkWidget *tmpItem, *tmpText;
	gint tmpIndex;
	gchar *subName, *subAuthor;

	(void) widget;
	(void) data;

	/* Freeze text-widget and delete the old text */
	tmpText = LUW("fileinfo_sub_info");

	if (xs_fileinfostil) {
		/* Get subtune number */
		tmpItem = gtk_menu_get_active(GTK_MENU(data));
		tmpIndex = g_list_index(GTK_MENU_SHELL(data)->children, tmpItem);

		/* Get subtune information */
		tmpNode = &xs_fileinfostil->subTunes[tmpIndex];
		subName = tmpNode->pName;
		subAuthor = tmpNode->pAuthor;

		/* Put in the new text, if available */
		if (tmpNode->pInfo) {
			gsize pInfo_utf8_size;
			gchar *pInfo_utf8 = g_locale_to_utf8( tmpNode->pInfo , strlen(tmpNode->pInfo) , NULL , &pInfo_utf8_size , NULL );
			gtk_text_buffer_set_text( GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(tmpText))),
			  pInfo_utf8, pInfo_utf8_size);
		}
	} else {
		/* We don't have any information */
		subName = NULL;
		subAuthor = NULL;
	}

	/* Get and set subtune information */
	gtk_entry_set_text(GTK_ENTRY(LUW("fileinfo_sub_name")), subName ? g_locale_to_utf8(subName,strlen(subName),NULL,NULL,NULL) : "");
	gtk_entry_set_text(GTK_ENTRY(LUW("fileinfo_sub_author")), subAuthor ? g_locale_to_utf8(subAuthor,strlen(subAuthor),NULL,NULL,NULL) : "");
}


void xs_fileinfo(gchar * pcFilename)
{
	GtkWidget *tmpMenuItem, *tmpMenu, *tmpOptionMenu;
	t_xs_tuneinfo *tmpInfo;
	t_xs_stil_subnode *tmpNode;
	gchar tmpStr[64];
	gint n;

	/* Current implementation leaves old fileinfo window untouched if
	 * no information can be found for the new file. Hmm...
	 */

	/* Get new tune information */
	XS_MUTEX_LOCK(xs_fileinfowin);
	XS_MUTEX_LOCK(xs_status);
	if ((tmpInfo = xs_status.sidPlayer->plrGetSIDInfo(pcFilename)) == NULL) {
		XS_MUTEX_UNLOCK(xs_fileinfowin);
		XS_MUTEX_UNLOCK(xs_status);
		return;
	}
	XS_MUTEX_UNLOCK(xs_status);

	xs_fileinfostil = xs_stil_get(pcFilename);

	/* Check if there already is an open fileinfo window */
	if (xs_fileinfowin) {
		/* Raise old window */
		gdk_window_raise(xs_fileinfowin->window);

		/* Delete items */
		tmpOptionMenu = LUW("fileinfo_sub_tune");
		gtk_widget_destroy(GTK_OPTION_MENU(tmpOptionMenu)->menu);
		GTK_OPTION_MENU(tmpOptionMenu)->menu = gtk_menu_new();
	} else {
		/* If not, create a new one */
		xs_fileinfowin = create_xs_fileinfowin();

		/* Connect additional signals */
		gtk_signal_connect(GTK_OBJECT(gtk_range_get_adjustment(GTK_RANGE(LUW("fileinfo_subctrl_adj")))),
				   "value_changed", GTK_SIGNAL_FUNC(xs_fileinfo_setsong), NULL);
	}


	/* Set the generic song information */
	gtk_entry_set_text(GTK_ENTRY(LUW("fileinfo_filename")), g_locale_to_utf8(pcFilename,strlen(pcFilename),NULL,NULL,NULL) );
	gtk_entry_set_text(GTK_ENTRY(LUW("fileinfo_songname")), g_locale_to_utf8(tmpInfo->sidName,strlen(tmpInfo->sidName),NULL,NULL,NULL) );
	gtk_entry_set_text(GTK_ENTRY(LUW("fileinfo_composer")), g_locale_to_utf8(tmpInfo->sidComposer,strlen(tmpInfo->sidComposer),NULL,NULL,NULL) );
	gtk_entry_set_text(GTK_ENTRY(LUW("fileinfo_copyright")), g_locale_to_utf8(tmpInfo->sidCopyright,strlen(tmpInfo->sidCopyright),NULL,NULL,NULL) );

	/* Main tune - the pseudo tune */
	tmpOptionMenu = LUW("fileinfo_sub_tune");
	tmpMenu = GTK_OPTION_MENU(tmpOptionMenu)->menu;

	tmpMenuItem = gtk_menu_item_new_with_label("General info");
	gtk_widget_show(tmpMenuItem);
	gtk_menu_append(GTK_MENU(tmpMenu), tmpMenuItem);
	gtk_signal_connect(GTK_OBJECT(tmpMenuItem), "activate", GTK_SIGNAL_FUNC(xs_fileinfo_subtune), tmpMenu);

	/* Other menu items */
	for (n = 1; n <= tmpInfo->nsubTunes; n++) {
		if (xs_fileinfostil) {
			snprintf(tmpStr, sizeof(tmpStr), "Tune #%i: ", n);
			tmpNode = &xs_fileinfostil->subTunes[n];
			if (tmpNode->pName)
				xs_pnstrcat(tmpStr, sizeof(tmpStr), tmpNode->pName);
			else if (tmpNode->pInfo)
				xs_pnstrcat(tmpStr, sizeof(tmpStr), tmpNode->pInfo);
			else
				xs_pnstrcat(tmpStr, sizeof(tmpStr), "---");
		} else {
			snprintf(tmpStr, sizeof(tmpStr), "Tune #%i", n);
		}

		tmpMenuItem = gtk_menu_item_new_with_label(tmpStr);
		gtk_widget_show(tmpMenuItem);
		gtk_menu_append(GTK_MENU(tmpMenu), tmpMenuItem);

		gtk_signal_connect(GTK_OBJECT(tmpMenuItem), "activate", GTK_SIGNAL_FUNC(xs_fileinfo_subtune), tmpMenu);
	}

	/* Set the subtune information */
	xs_fileinfo_subtune(NULL, tmpMenu);

	/* Free temporary tuneinfo */
	xs_tuneinfo_free(tmpInfo);

	/* Show the window */
	gtk_widget_show(xs_fileinfowin);

	XS_MUTEX_UNLOCK(xs_fileinfowin);

	xs_fileinfo_update();
}
