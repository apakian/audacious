/*
 *  Copyright (c) 2006 William Pitcock <nenolod -at- nenolod.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <glib.h>

struct _InterfacePlugin {
    gchar *name;
    void (*init) (void);
    void (*cleanup) (void);
    void (*about) (void);
    void (*configure) (void);
    void (*disable_plugin) (struct _VisPlugin *);
    void (*playback_start) (void);
    void (*playback_stop) (void);
    void (*dump_pcm_data) (gint16 pcm_data[2][512]);
    void (*dump_freq_data) (gint16 freq_data[2][256]);
    void (*redraw) (void);
};

typedef struct _InterfacePlugin InterfacePlugin;

extern void register_interface_plugin(InterfacePlugin *);
extern void start_interface_plugin(InterfacePlugin *);

#endif