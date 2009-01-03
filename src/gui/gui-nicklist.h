/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_GUI_NICKLIST_H
#define __WEECHAT_GUI_NICKLIST_H 1

struct t_gui_buffer;
struct t_infolist;

struct t_gui_nick_group
{
    char *name;                        /* group name                        */
    char *color;                       /* color for group in nicklist       */
    int visible;                       /* 1 if group is displayed           */
    int level;                         /* group level (root is 0)           */
    struct t_gui_nick_group *parent;   /* parent                            */
    struct t_gui_nick_group *childs;     /* childs                          */
    struct t_gui_nick_group *last_child; /* last child                      */
    struct t_gui_nick *nicks;          /* nicks for group                   */
    struct t_gui_nick *last_nick;      /* last nick for group               */
    struct t_gui_nick_group *prev_group; /* link to previous group          */
    struct t_gui_nick_group *next_group; /* link to next group              */
};

struct t_gui_nick
{
    struct t_gui_nick_group *group;    /* group which contains nick         */
    char *name;                        /* nick name                         */
    char *color;                       /* color for nick in nicklist        */
    char prefix;                       /* prefix for nick (for admins, ..)  */
    char *prefix_color;                /* color for prefix                  */
    int visible;                       /* 1 if nick is displayed            */
    struct t_gui_nick *prev_nick;      /* link to previous nick             */
    struct t_gui_nick *next_nick;      /* link to next nick                 */
};

/* nicklist functions */

extern struct t_gui_nick_group *gui_nicklist_search_group (struct t_gui_buffer *buffer,
                                                           struct t_gui_nick_group *from_group,
                                                           const char *name);
extern struct t_gui_nick_group *gui_nicklist_add_group (struct t_gui_buffer *buffer,
                                                        struct t_gui_nick_group *parent_group,
                                                        const char *name, const char *color,
                                                        int visible);
extern struct t_gui_nick *gui_nicklist_search_nick (struct t_gui_buffer *buffer,
                                                    struct t_gui_nick_group *from_group,
                                                    const char *name);
extern struct t_gui_nick *gui_nicklist_add_nick (struct t_gui_buffer *buffer,
                                                 struct t_gui_nick_group *group,
                                                 const char *name, const char *color,
                                                 char prefix, const char *prefix_color,
                                                 int visible);
extern void gui_nicklist_remove_group (struct t_gui_buffer *buffer,
                                       struct t_gui_nick_group *group);
extern void gui_nicklist_remove_nick (struct t_gui_buffer *buffer,
                                      struct t_gui_nick *nick);
extern void gui_nicklist_remove_all (struct t_gui_buffer *buffer);
extern void gui_nicklist_get_next_item (struct t_gui_buffer *buffer,
                                        struct t_gui_nick_group **group,
                                        struct t_gui_nick **nick);
extern char *gui_nicklist_get_group_start (const char *name);
extern int gui_nicklist_get_max_length (struct t_gui_buffer *buffer,
                                        struct t_gui_nick_group *group);
extern void gui_nicklist_compute_visible_count (struct t_gui_buffer *buffer,
                                                struct t_gui_nick_group *group);
extern int gui_nicklist_add_to_infolist (struct t_infolist *infolist,
                                         struct t_gui_buffer *buffer);
extern void gui_nicklist_print_log (struct t_gui_nick_group *group, int indent);

/* nicklist functions (GUI dependent) */

extern int gui_nicklist_draw (struct t_gui_buffer *buffer, int erase);

#endif /* gui-nicklist.h */
