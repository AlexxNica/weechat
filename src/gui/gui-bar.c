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

/* gui-bar.c: bar functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-bar.h"
#include "gui-bar-item.h"
#include "gui-bar-window.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-window.h"


char *gui_bar_option_string[GUI_BAR_NUM_OPTIONS] =
{ "hidden", "priority", "type", "conditions", "position", "filling_top_bottom",
  "filling_left_right", "size", "size_max", "color_fg", "color_delim",
  "color_bg", "separator", "items" };
char *gui_bar_type_string[GUI_BAR_NUM_TYPES] =
{ "root", "window" };
char *gui_bar_position_string[GUI_BAR_NUM_POSITIONS] =
{ "bottom", "top", "left", "right" };
char *gui_bar_filling_string[GUI_BAR_NUM_FILLING] =
{ "horizontal", "vertical", "columns_horizontal", "columns_vertical" };

struct t_gui_bar *gui_bars = NULL;         /* first bar                     */
struct t_gui_bar *last_gui_bar = NULL;     /* last bar                      */

struct t_gui_bar *gui_temp_bars = NULL;    /* bars used when reading config */
struct t_gui_bar *last_gui_temp_bar = NULL;


/*
 * gui_bar_valid: check if a bar pointer exists
 *                return 1 if bar exists
 *                       0 if bar is not found
 */

int
gui_bar_valid (struct t_gui_bar *bar)
{
    struct t_gui_bar *ptr_bar;
    
    if (!bar)
        return 0;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (ptr_bar == bar)
            return 1;
    }
    
    /* bar not found */
    return 0;
}

/*
 * gui_bar_search_option search a bar option name
 *                       return index of option in array
 *                       "gui_bar_option_string", or -1 if not found
 */

int
gui_bar_search_option (const char *option_name)
{
    int i;
    
    if (!option_name)
        return -1;
    
    for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
    {
        if (string_strcasecmp (gui_bar_option_string[i], option_name) == 0)
            return i;
    }
    
    /* bar option not found */
    return -1;
}

/*
 * gui_bar_search_type: search type number with string
 *                      return -1 if type is not found
 */

int
gui_bar_search_type (const char *type)
{
    int i;
    
    if (!type)
        return -1;
    
    for (i = 0; i < GUI_BAR_NUM_TYPES; i++)
    {
        if (string_strcasecmp (gui_bar_type_string[i], type) == 0)
            return i;
    }
    
    /* type not found */
    return -1;
}

/*
 * gui_bar_search_position: search position number with string
 *                          return -1 if type is not found
 */

int
gui_bar_search_position (const char *position)
{
    int i;
    
    if (!position)
        return -1;
    
    for (i = 0; i < GUI_BAR_NUM_POSITIONS; i++)
    {
        if (string_strcasecmp (gui_bar_position_string[i], position) == 0)
            return i;
    }
    
    /* position not found */
    return -1;
}

/*
 * gui_bar_get_min_width: return minimum width of a bar window displayed for
 *                        a bar
 *                        for example, if a bar is displayed in 3 windows,
 *                        this function return min width of these 3 bar windows
 */

int
gui_bar_get_min_width (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    int min_width;
    
    if (CONFIG_INTEGER(bar->type) == GUI_BAR_TYPE_ROOT)
        return bar->bar_window->width;
    
    min_width = INT_MAX;
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        for (ptr_bar_win = ptr_win->bar_windows; ptr_bar_win;
             ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            if (ptr_bar_win->bar == bar)
            {
                if (ptr_bar_win->width < min_width)
                    min_width = ptr_bar_win->width;
            }
        }
    }
    
    if (min_width == INT_MAX)
        return 0;
    
    return min_width;
}

/*
 * gui_bar_get_min_height: return minimum height of a bar window displayed for
 *                         a bar
 *                         for example, if a bar is displayed in 3 windows,
 *                         this function return min width of these 3 bar windows
 */

int
gui_bar_get_min_height (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    int min_height;
    
    if (CONFIG_INTEGER(bar->type) == GUI_BAR_TYPE_ROOT)
        return bar->bar_window->height;
    
    min_height = INT_MAX;
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        for (ptr_bar_win = ptr_win->bar_windows; ptr_bar_win;
             ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            if (ptr_bar_win->bar == bar)
            {
                if (ptr_bar_win->height < min_height)
                    min_height = ptr_bar_win->height;
            }
        }
    }
    
    if (min_height == INT_MAX)
        return 0;
    
    return min_height;
}

/*
 * gui_bar_check_size_add: check if "add_size" is ok for bar
 *                         return 1 if new size is ok
 *                                0 if new size is too big
 */

int
gui_bar_check_size_add (struct t_gui_bar *bar, int add_size)
{
    struct t_gui_window *ptr_window;
    int sub_width, sub_height;
    
    sub_width = 0;
    sub_height = 0;
    
    switch (CONFIG_INTEGER(bar->position))
    {
        case GUI_BAR_POSITION_BOTTOM:
        case GUI_BAR_POSITION_TOP:
            sub_height = add_size;
            break;
        case GUI_BAR_POSITION_LEFT:
        case GUI_BAR_POSITION_RIGHT:
            sub_width = add_size;
            break;
        case GUI_BAR_NUM_POSITIONS:
            break;
    }
    
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if ((CONFIG_INTEGER(bar->type) == GUI_BAR_TYPE_ROOT)
            || (gui_bar_window_search_bar (ptr_window, bar)))
        {
            if ((ptr_window->win_chat_width - sub_width < GUI_WINDOW_CHAT_MIN_WIDTH)
                || (ptr_window->win_chat_height - sub_height < GUI_WINDOW_CHAT_MIN_HEIGHT))
                return 0;
        }
    }
    
    /* new size ok */
    return 1;
}

/*
 * gui_bar_get_filling: return filling option for bar, according to filling
 *                      for current bar position
 */

enum t_gui_bar_filling
gui_bar_get_filling (struct t_gui_bar *bar)
{
    if ((CONFIG_INTEGER(bar->position) == GUI_BAR_POSITION_BOTTOM)
        || (CONFIG_INTEGER(bar->position) == GUI_BAR_POSITION_TOP))
        return CONFIG_INTEGER(bar->filling_top_bottom);
    
    return CONFIG_INTEGER(bar->filling_left_right);
}

/*
 * gui_bar_get_item_index: return index of item and sub item in bar (position
 *                         of item in items list)
 *                         for example, if items are:
 *                            item1,sub1+sub2+sub3,item3
 *                         index of sub3 is 1, sub index is 2
 *                         return -1 for index and sub index if item is not
 *                         found in bar
 */

void
gui_bar_get_item_index (struct t_gui_bar *bar, const char *item_name,
                        int *index_item, int *index_subitem)
{
    int i, j;
    
    *index_item = -1;
    *index_subitem = -1;
    
    if (!bar || !item_name || !item_name[0])
        return;
    
    for (i = 0; i < bar->items_count; i++)
    {
        for (j = 0; j < bar->items_subcount[i]; j++)
        {
            /* skip non letters chars at beginning (prefix) */
            if (gui_bar_item_string_is_item (bar->items_array[i][j], item_name))
            {
                *index_item = i;
                *index_subitem = j;
                return;
            }
        }
    }
    
    /* item is not in bar */
}

/*
 * gui_bar_find_pos: find position for a bar in list (keeping list sorted
 *                   by priority)
 */

struct t_gui_bar *
gui_bar_find_pos (struct t_gui_bar *bar)
{
    struct t_gui_bar *ptr_bar;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (CONFIG_INTEGER(bar->priority) >= CONFIG_INTEGER(ptr_bar->priority))
            return ptr_bar;
    }
    
    /* bar not found, add to end of list */
    return NULL;
}

/*
 * gui_bar_insert: insert a bar to the list (at good position, according to
 *                 priority)
 */

void
gui_bar_insert (struct t_gui_bar *bar)
{
    struct t_gui_bar *pos_bar;
    
    if (gui_bars)
    {
        pos_bar = gui_bar_find_pos (bar);
        if (pos_bar)
        {
            /* insert bar into the list (before position found) */
            bar->prev_bar = pos_bar->prev_bar;
            bar->next_bar = pos_bar;
            if (pos_bar->prev_bar)
                (pos_bar->prev_bar)->next_bar = bar;
            else
                gui_bars = bar;
            pos_bar->prev_bar = bar;
        }
        else
        {
            /* add bar to the end */
            bar->prev_bar = last_gui_bar;
            bar->next_bar = NULL;
            last_gui_bar->next_bar = bar;
            last_gui_bar = bar;
        }
    }
    else
    {
        bar->prev_bar = NULL;
        bar->next_bar = NULL;
        gui_bars = bar;
        last_gui_bar = bar;
    }
}

/*
 * gui_bar_check_conditions_for_window: return 1 if bar should be displayed in
 *                                      this window, according to condition(s)
 *                                      on bar
 */

int
gui_bar_check_conditions_for_window (struct t_gui_bar *bar,
                                     struct t_gui_window *window)
{
    int i;

    for (i = 0; i < bar->conditions_count; i++)
    {
        if (string_strcasecmp (bar->conditions_array[i], "active") == 0)
        {
            if (gui_current_window && (gui_current_window != window))
                return 0;
        }
        else if (string_strcasecmp (bar->conditions_array[i], "inactive") == 0)
        {
            if (!gui_current_window || (gui_current_window == window))
                return 0;
        }
        else if (string_strcasecmp (bar->conditions_array[i], "nicklist") == 0)
        {
            if (window->buffer && !window->buffer->nicklist)
                return 0;
        }
    }
    
    return 1;
}

/*
 * gui_bar_root_get_size: get total bar size ("root" type) for a position
 */

int
gui_bar_root_get_size (struct t_gui_bar *bar, enum t_gui_bar_position position)
{
    struct t_gui_bar *ptr_bar;
    int total_size;
    
    total_size = 0;
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (bar && (ptr_bar == bar))
            return total_size;

        if (!CONFIG_BOOLEAN(ptr_bar->hidden))
        {
            if ((CONFIG_INTEGER(ptr_bar->type) == GUI_BAR_TYPE_ROOT)
                && (CONFIG_INTEGER(ptr_bar->position) == (int)position))
            {
                total_size += gui_bar_window_get_current_size (ptr_bar->bar_window);
                if (CONFIG_INTEGER(ptr_bar->separator))
                    total_size++;
            }
        }
    }
    return total_size;
}

/*
 * gui_bar_search: search a bar by name
 */

struct t_gui_bar *
gui_bar_search (const char *name)
{
    struct t_gui_bar *ptr_bar;
    
    if (!name || !name[0])
        return NULL;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (strcmp (ptr_bar->name, name) == 0)
            return ptr_bar;
    }
    
    /* bar not found */
    return NULL;
}

/*
 * gui_bar_search_with_option_name: search a bar with name of option
 *                                  (like "uptime.type")
 */

struct t_gui_bar *
gui_bar_search_with_option_name (const char *option_name)
{
    char *bar_name, *pos_option;
    struct t_gui_bar *ptr_bar;
    
    ptr_bar = NULL;
    
    pos_option = strchr (option_name, '.');
    if (pos_option)
    {
        bar_name = string_strndup (option_name, pos_option - option_name);
        if (bar_name)
        {
            for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
            {
                if (strcmp (ptr_bar->name, bar_name) == 0)
                    break;
            }
            free (bar_name);
        }
    }
    
    return ptr_bar;
}

/*
 * gui_bar_content_build_bar_windows: rebuild content of bar windows for a bar
 */

void
gui_bar_content_build_bar_windows (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_window;
    struct t_gui_bar_window *ptr_bar_window;
    
    if (!bar)
        return;
    
    if (bar->bar_window)
    {
        gui_bar_window_content_build (bar->bar_window, NULL);
    }
    else
    {
        for (ptr_window = gui_windows; ptr_window;
             ptr_window = ptr_window->next_window)
        {
            for (ptr_bar_window = ptr_window->bar_windows; ptr_bar_window;
                 ptr_bar_window = ptr_bar_window->next_bar_window)
            {
                if (ptr_bar_window->bar == bar)
                    gui_bar_window_content_build (ptr_bar_window, ptr_window);
            }
        }
    }
}

/*
 * gui_bar_ask_refresh: ask refresh for bar
 */

void
gui_bar_ask_refresh (struct t_gui_bar *bar)
{
    bar->bar_refresh_needed = 1;
}

/*
 * gui_bar_refresh: ask for bar refresh on screen (for all windows where bar is)
 */

void
gui_bar_refresh (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    
    if (CONFIG_INTEGER(bar->type) == GUI_BAR_TYPE_ROOT)
        gui_window_refresh_needed = 1;
    else
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            if (gui_bar_window_search_bar (ptr_win, bar))
                ptr_win->refresh_needed = 1;
        }
    }
}

/*
 * gui_bar_draw: draw a bar
 */

void
gui_bar_draw (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    
    if (CONFIG_BOOLEAN(bar->hidden))
        return;
    
    if (bar->bar_window)
    {
        /* root bar */
        gui_bar_window_draw (bar->bar_window, NULL);
    }
    else
    {
        /* bar on each window */
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            for (ptr_bar_win = ptr_win->bar_windows; ptr_bar_win;
                 ptr_bar_win = ptr_bar_win->next_bar_window)
            {
                if (ptr_bar_win->bar == bar)
                {
                    gui_bar_window_draw (ptr_bar_win, ptr_win);
                }
            }
        }
    }
    
    bar->bar_refresh_needed = 0;
}

/*
 * gui_bar_free_items_array: free array with items for a bar
 */

void
gui_bar_free_items_array (struct t_gui_bar *bar)
{
    int i;
    
    for (i = 0; i < bar->items_count; i++)
    {
        if (bar->items_array[i])
            string_free_exploded (bar->items_array[i]);
    }
    if (bar->items_array)
    {
        free (bar->items_array);
        bar->items_array = NULL;
    }
    if (bar->items_subcount)
    {
        free (bar->items_subcount);
        bar->items_subcount = NULL;
    }
    bar->items_count = 0;
}

/*
 * gui_bar_set_items_array: build array with items for a bar
 */

void
gui_bar_set_items_array (struct t_gui_bar *bar, const char *items)
{
    int i, count;
    char **tmp_array;
    
    gui_bar_free_items_array (bar);
    
    if (items && items[0])
    {
        tmp_array = string_explode (items, ",", 0, 0, &count);
        if (count > 0)
        {
            bar->items_count = count;
            bar->items_subcount = malloc (count * sizeof (*bar->items_subcount));
            bar->items_array = malloc (count * sizeof (*bar->items_array));
            for (i = 0; i < count; i++)
            {
                bar->items_array[i] = string_explode (tmp_array[i], "+", 0, 0,
                                                      &(bar->items_subcount[i]));
            }
        }
    }
}

/*
 * gui_bar_config_check_type: callback for checking bar type before changing it
 */

int
gui_bar_config_check_type (void *data, struct t_config_option *option,
                           const char *value)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    (void) value;

    gui_chat_printf (NULL,
                     _("%sUnable to change bar type: you must delete bar "
                       "and create another to do that"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
    return 0;
}

/*
 * gui_bar_config_change_hidden: callback when "hidden" flag is changed
 */

void
gui_bar_config_change_hidden (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    struct t_gui_window *ptr_win;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        /* free bar windows */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            gui_bar_free_bar_windows (ptr_bar);
        }
        
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
            {
                if (!CONFIG_BOOLEAN(ptr_bar->hidden)
                    && (CONFIG_INTEGER(ptr_bar->type) != GUI_BAR_TYPE_ROOT))
                {
                    gui_bar_window_new (ptr_bar, ptr_win);
                }
            }
        }
    }
    
    gui_window_refresh_needed = 1;
}

/*
 * gui_bar_config_change_priority: callback when priority is changed
 */

void
gui_bar_config_change_priority (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    struct t_gui_window *ptr_win;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        /* remove bar from list */
        if (ptr_bar == gui_bars)
        {
            gui_bars = ptr_bar->next_bar;
            gui_bars->prev_bar = NULL;
        }
        if (ptr_bar == last_gui_bar)
        {
            last_gui_bar = ptr_bar->prev_bar;
            last_gui_bar->next_bar = NULL;
        }
        if (ptr_bar->prev_bar)
            (ptr_bar->prev_bar)->next_bar = ptr_bar->next_bar;
        if (ptr_bar->next_bar)
            (ptr_bar->next_bar)->prev_bar = ptr_bar->prev_bar;
        
        gui_bar_insert (ptr_bar);
        
        /* free bar windows */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            gui_bar_free_bar_windows (ptr_bar);
        }
        
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
            {
                if (!CONFIG_BOOLEAN(ptr_bar->hidden)
                    && (CONFIG_INTEGER(ptr_bar->type) != GUI_BAR_TYPE_ROOT))
                {
                    gui_bar_window_new (ptr_bar, ptr_win);
                }
            }
        }
    }
    
    gui_window_refresh_needed = 1;
}

/*
 * gui_bar_config_change_conditions: callback when conditions is changed
 */

void
gui_bar_config_change_conditions (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        if (ptr_bar->conditions_array)
            string_free_exploded (ptr_bar->conditions_array);
        
        if (CONFIG_STRING(ptr_bar->conditions) && CONFIG_STRING(ptr_bar->conditions)[0])
        {
            ptr_bar->conditions_array = string_explode (CONFIG_STRING(ptr_bar->conditions),
                                                        ",", 0, 0,
                                                        &ptr_bar->conditions_count);
        }
        else
        {
            ptr_bar->conditions_count = 0;
            ptr_bar->conditions_array = NULL;
        }
    }
    
    gui_window_refresh_needed = 1;
}

/*
 * gui_bar_config_change_position: callback when position is changed
 */

void
gui_bar_config_change_position (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->hidden))
        gui_bar_refresh (ptr_bar);
    
    gui_window_refresh_needed = 1;
}

/*
 * gui_bar_config_change_filling: callback when filling is changed
 */

void
gui_bar_config_change_filling (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->hidden))
        gui_bar_refresh (ptr_bar);
    
    gui_window_refresh_needed = 1;
}

/*
 * gui_bar_config_check_size: callback for checking bar size before changing it
 */

int
gui_bar_config_check_size (void *data, struct t_config_option *option,
                           const char *value)
{
    struct t_gui_bar *ptr_bar;
    long number;
    char *error;
    int new_value;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        new_value = -1;
        if (strncmp (value, "++", 2) == 0)
        {
            error = NULL;
            number = strtol (value + 2, &error, 10);
            if (error && !error[0])
            {
                new_value = CONFIG_INTEGER(ptr_bar->size) + number;
            }
        }
        else if (strncmp (value, "--", 2) == 0)
        {
            error = NULL;
            number = strtol (value + 2, &error, 10);
            if (error && !error[0])
            {
                new_value = CONFIG_INTEGER(ptr_bar->size) - number;
            }
        }
        else
        {
            error = NULL;
            number = strtol (value, &error, 10);
            if (error && !error[0])
            {
                new_value = number;
            }
        }
        if (new_value < 0)
            return 0;
        
        if ((new_value > 0) &&
            ((CONFIG_INTEGER(ptr_bar->size) == 0)
             || (new_value > CONFIG_INTEGER(ptr_bar->size))))
        {
            if (!CONFIG_BOOLEAN(ptr_bar->hidden)
                && !gui_bar_check_size_add (ptr_bar,
                                            new_value - CONFIG_INTEGER(ptr_bar->size)))
                return 0;
        }
        
        return 1;
    }
    
    return 0;
}

/*
 * gui_bar_config_change_size: callback when size is changed
 */

void
gui_bar_config_change_size (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        gui_bar_window_set_current_size (ptr_bar,
                                         CONFIG_INTEGER(ptr_bar->size));
        gui_window_refresh_needed = 1;
    }
}

/*
 * gui_bar_config_change_size_max: callback when max size is changed
 */

void
gui_bar_config_change_size_max (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    gui_window_refresh_needed = 1;
}

/*
 * gui_bar_config_change_color: callback when color (fg or bg) is changed
 */

void
gui_bar_config_change_color (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->hidden))
        gui_bar_refresh (ptr_bar);
}

/*
 * gui_bar_config_change_separator: callback when separator is changed
 */

void
gui_bar_config_change_separator (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->hidden))
        gui_bar_refresh (ptr_bar);
}

/*
 * gui_bar_config_change_items: callback when items is changed
 */

void
gui_bar_config_change_items (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        gui_bar_set_items_array (ptr_bar, CONFIG_STRING(ptr_bar->items));
        
        if (!CONFIG_BOOLEAN(ptr_bar->hidden))
        {
            gui_bar_content_build_bar_windows (ptr_bar);
            gui_bar_ask_refresh (ptr_bar);
        }
    }
}

/*
 * gui_bar_set_name: set name for a bar
 */

void
gui_bar_set_name (struct t_gui_bar *bar, const char *name)
{
    int length;
    char *option_name;
    
    if (!name || !name[0])
        return;
    
    length = strlen (name) + 64;
    option_name = malloc (length);
    if (option_name)
    {
        snprintf (option_name, length, "%s.hidden", name);
        config_file_option_rename (bar->hidden, option_name);
        snprintf (option_name, length, "%s.priority", name);
        config_file_option_rename (bar->priority, option_name);
        snprintf (option_name, length, "%s.type", name);
        config_file_option_rename (bar->type, option_name);
        snprintf (option_name, length, "%s.conditions", name);
        config_file_option_rename (bar->conditions, option_name);
        snprintf (option_name, length, "%s.position", name);
        config_file_option_rename (bar->position, option_name);
        snprintf (option_name, length, "%s.filling_top_bottom", name);
        config_file_option_rename (bar->filling_top_bottom, option_name);
        snprintf (option_name, length, "%s.filling_left_right", name);
        config_file_option_rename (bar->filling_left_right, option_name);
        snprintf (option_name, length, "%s.size", name);
        config_file_option_rename (bar->size, option_name);
        snprintf (option_name, length, "%s.size_max", name);
        config_file_option_rename (bar->size_max, option_name);
        snprintf (option_name, length, "%s.color_fg", name);
        config_file_option_rename (bar->color_fg, option_name);
        snprintf (option_name, length, "%s.color_delim", name);
        config_file_option_rename (bar->color_delim, option_name);
        snprintf (option_name, length, "%s.color_bg", name);
        config_file_option_rename (bar->color_bg, option_name);
        snprintf (option_name, length, "%s.separator", name);
        config_file_option_rename (bar->separator, option_name);
        snprintf (option_name, length, "%s.items", name);
        config_file_option_rename (bar->items, option_name);
        
        if (bar->name)
            free (bar->name);
        bar->name = strdup (name);
        
        free (option_name);
    }
}

/*
 * gui_bar_set_priority: set priority for a bar
 */

void
gui_bar_set_priority (struct t_gui_bar *bar, const char *priority)
{
    long number;
    char *error;
    
    error = NULL;
    number = strtol (priority, &error, 10);
    if (error && !error[0])
    {
        if (number < 0)
            number = 0;
        
        /* bar number is already ok? */
        if (number == CONFIG_INTEGER(bar->priority))
            return;
        
        config_file_option_set (bar->priority, priority, 1);
    }
}

/*
 * gui_bar_set_position: set position for a bar
 */

void
gui_bar_set_position (struct t_gui_bar *bar, const char *position)
{
    int position_value;
    
    if (!position || !position[0])
        return;
    
    position_value = gui_bar_search_position (position);
    if ((position_value >= 0) && (CONFIG_INTEGER(bar->position) != position_value))
    {
        config_file_option_set (bar->position, position, 1);
    }
}

/*
 * gui_bar_set_size: set size for a bar
 */

void
gui_bar_set_size (struct t_gui_bar *bar, const char *size)
{
    long number;
    char *error, value[32];
    int new_size;
    
    error = NULL;
    number = strtol (((size[0] == '+') || (size[0] == '-')) ?
                     size + 1 : size,
                     &error,
                     10);
    if (error && !error[0])
    {
        new_size = number;
        if (size[0] == '+')
            new_size = CONFIG_INTEGER(bar->size) + new_size;
        else if (value[0] == '-')
            new_size = CONFIG_INTEGER(bar->size) - new_size;
        if ((size[0] == '-') && (new_size < 1))
            return;
        if (new_size < 0)
            return;
        
        /* check if new size is ok if it's more than before */
        if ((new_size != 0) &&
            ((CONFIG_INTEGER(bar->size) == 0)
             || (new_size > CONFIG_INTEGER(bar->size))))
        {
            if (!gui_bar_check_size_add (bar,
                                         new_size - CONFIG_INTEGER(bar->size)))
                return;
        }
        
        snprintf (value, sizeof (value), "%d", new_size);
        config_file_option_set (bar->size, value, 1);

        gui_bar_window_set_current_size (bar, new_size);
    }
}

/*
 * gui_bar_set_size_max: set max size for a bar
 */

void
gui_bar_set_size_max (struct t_gui_bar *bar, const char *size)
{
    long number;
    char *error, value[32];
    
    error = NULL;
    number = strtol (size, &error, 10);
    if (error && !error[0])
    {
        if (number < 0)
            return;
        
        snprintf (value, sizeof (value), "%ld", number);
        config_file_option_set (bar->size_max, value, 1);

        if ((number > 0) &&
            ((CONFIG_INTEGER(bar->size) == 0)
             || (number < CONFIG_INTEGER(bar->size))))
            gui_bar_set_size (bar, value);
    }
}

/*
 * gui_bar_set: set a property for a bar
 *              return: 1 if ok, 0 if error
 */

int
gui_bar_set (struct t_gui_bar *bar, const char *property, const char *value)
{
    if (!bar || !property || !value)
        return 0;
    
    if (string_strcasecmp (property, "name") == 0)
    {
        gui_bar_set_name (bar, value);
        return 1;
    }
    else if (string_strcasecmp (property, "hidden") == 0)
    {
        config_file_option_set (bar->hidden, value, 1);
        return 1;
    }
    else if (string_strcasecmp (property, "priority") == 0)
    {
        gui_bar_set_priority (bar, value);
        return 1;
    }
    else if (string_strcasecmp (property, "conditions") == 0)
    {
        config_file_option_set (bar->conditions, value, 1);
        return 1;
    }
    else if (string_strcasecmp (property, "position") == 0)
    {
        gui_bar_set_position (bar, value);
        return 1;
    }
    else if (string_strcasecmp (property, "filling_top_bottom") == 0)
    {
        config_file_option_set (bar->filling_top_bottom, value, 1);
        return 1;
    }
    else if (string_strcasecmp (property, "filling_left_right") == 0)
    {
        config_file_option_set (bar->filling_left_right, value, 1);
        return 1;
    }
    else if (string_strcasecmp (property, "size") == 0)
    {
        gui_bar_set_size (bar, value);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (string_strcasecmp (property, "size_max") == 0)
    {
        gui_bar_set_size_max (bar, value);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (string_strcasecmp (property, "color_fg") == 0)
    {
        config_file_option_set (bar->color_fg, value, 1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (string_strcasecmp (property, "color_delim") == 0)
    {
        config_file_option_set (bar->color_delim, value, 1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (string_strcasecmp (property, "color_bg") == 0)
    {
        config_file_option_set (bar->color_bg, value, 1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (string_strcasecmp (property, "separator") == 0)
    {
        config_file_option_set (bar->separator,
                                (strcmp (value, "1") == 0) ? "on" : "off",
                                1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (string_strcasecmp (property, "items") == 0)
    {
        config_file_option_set (bar->items, value, 1);
        gui_bar_draw (bar);
        return 1;
    }
    
    return 0;
}

/*
 * gui_bar_create_option: create an option for a bar
 */

struct t_config_option *
gui_bar_create_option (const char *bar_name, int index_option, const char *value)
{
    struct t_config_option *ptr_option;
    int length;
    char *option_name;
    
    ptr_option = NULL;
    
    length = strlen (bar_name) + 1 +
        strlen (gui_bar_option_string[index_option]) + 1;
    option_name = malloc (length);
    if (option_name)
    {
        snprintf (option_name, length, "%s.%s",
                  bar_name, gui_bar_option_string[index_option]);
        
        switch (index_option)
        {
            case GUI_BAR_OPTION_HIDDEN:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "boolean",
                    N_("true if bar is hidden, false if it is displayed"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_hidden, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_PRIORITY:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("bar priority (high number means bar displayed first)"),
                    NULL, 0, INT_MAX, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_priority, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_TYPE:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("bar type (root, window, window_active, window_inactive)"),
                    "root|window|window_active|window_inactive", 0, 0, value, NULL, 0,
                    &gui_bar_config_check_type, NULL, NULL, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_CONDITIONS:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "string",
                    N_("condition(s) for displaying bar (for bars of type "
                       "\"window\")"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_conditions, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_POSITION:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("bar position (bottom, top, left, right)"),
                    "bottom|top|left|right", 0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_position, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_FILLING_TOP_BOTTOM:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("bar filling direction (\"horizontal\" (from left to "
                       "right) or \"vertical\" (from top to bottom)) when bar "
                       "position is top or bottom"),
                    "horizontal|vertical|columns_horizontal|columns_vertical",
                    0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_filling, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_FILLING_LEFT_RIGHT:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("bar filling direction (\"horizontal\" (from left to "
                       "right) or \"vertical\" (from top to bottom)) when bar "
                       "position is left or right"),
                    "horizontal|vertical|columns_horizontal|columns_vertical",
                    0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_filling, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_SIZE:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("bar size in chars (0 = auto size)"),
                    NULL, 0, INT_MAX, value, NULL, 0,
                    &gui_bar_config_check_size, NULL,
                    &gui_bar_config_change_size, NULL,
                    NULL, NULL);
                break;
            case GUI_BAR_OPTION_SIZE_MAX:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("max bar size in chars (0 = no limit)"),
                    NULL, 0, INT_MAX, value, NULL, 0,
                    NULL, NULL,
                    &gui_bar_config_change_size_max, NULL,
                    NULL, NULL);
                break;
            case GUI_BAR_OPTION_COLOR_FG:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "color",
                    N_("default text color for bar"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL,
                    &gui_bar_config_change_color, NULL,
                    NULL, NULL);
                break;
            case GUI_BAR_OPTION_COLOR_DELIM:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "color",
                    N_("default delimiter color for bar"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL,
                    &gui_bar_config_change_color, NULL,
                    NULL, NULL);
                break;
            case GUI_BAR_OPTION_COLOR_BG:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "color",
                    N_("default background color for bar"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL,
                    &gui_bar_config_change_color, NULL,
                    NULL, NULL);
                break;
            case GUI_BAR_OPTION_SEPARATOR:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "boolean",
                    N_("separator line between bar and other bars/windows"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_separator, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_ITEMS:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "string",
                    N_("items of bar"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_items, NULL, NULL, NULL);
                break;
            case GUI_BAR_NUM_OPTIONS:
                break;
        }
        free (option_name);
    }
    
    return ptr_option;
}

/*
 * gui_bar_create_option_temp: create option for a temporary bar (when reading
 *                             config file)
 */

void
gui_bar_create_option_temp (struct t_gui_bar *temp_bar, int index_option,
                            const char *value)
{
    struct t_config_option *new_option;
    
    new_option = gui_bar_create_option (temp_bar->name,
                                        index_option,
                                        value);
    if (new_option)
    {
        switch (index_option)
        {
            case GUI_BAR_OPTION_HIDDEN:
                temp_bar->hidden = new_option;
                break;
            case GUI_BAR_OPTION_PRIORITY:
                temp_bar->priority = new_option;
                break;
            case GUI_BAR_OPTION_TYPE:
                temp_bar->type = new_option;
                break;
            case GUI_BAR_OPTION_CONDITIONS:
                temp_bar->conditions = new_option;
                break;
            case GUI_BAR_OPTION_POSITION:
                temp_bar->position = new_option;
                break;
            case GUI_BAR_OPTION_FILLING_TOP_BOTTOM:
                temp_bar->filling_top_bottom = new_option;
                break;
            case GUI_BAR_OPTION_FILLING_LEFT_RIGHT:
                temp_bar->filling_left_right = new_option;
                break;
            case GUI_BAR_OPTION_SIZE:
                temp_bar->size = new_option;
                break;
            case GUI_BAR_OPTION_SIZE_MAX:
                temp_bar->size_max = new_option;
                break;
            case GUI_BAR_OPTION_COLOR_FG:
                temp_bar->color_fg = new_option;
                break;
            case GUI_BAR_OPTION_COLOR_DELIM:
                temp_bar->color_delim = new_option;
                break;
            case GUI_BAR_OPTION_COLOR_BG:
                temp_bar->color_bg = new_option;
                break;
            case GUI_BAR_OPTION_SEPARATOR:
                temp_bar->separator = new_option;
                break;
            case GUI_BAR_OPTION_ITEMS:
                temp_bar->items = new_option;
                break;
        }
    }
}

/*
 * gui_bar_alloc: allocate and initialize new bar structure
 */

struct t_gui_bar *
gui_bar_alloc (const char *name)
{
    struct t_gui_bar *new_bar;

    new_bar = malloc (sizeof (*new_bar));
    if (new_bar)
    {
        new_bar->name = strdup (name);
        new_bar->hidden = NULL;
        new_bar->priority = NULL;
        new_bar->type = NULL;
        new_bar->conditions = NULL;
        new_bar->position = NULL;
        new_bar->filling_top_bottom = NULL;
        new_bar->filling_left_right = NULL;
        new_bar->size = NULL;
        new_bar->size_max = NULL;
        new_bar->color_fg = NULL;
        new_bar->color_delim = NULL;
        new_bar->color_bg = NULL;
        new_bar->separator = NULL;
        new_bar->items = NULL;
        new_bar->conditions_count = 0;
        new_bar->conditions_array = NULL;
        new_bar->items_count = 0;
        new_bar->items_array = NULL;
        new_bar->bar_window = NULL;
        new_bar->bar_refresh_needed = 0;
        new_bar->prev_bar = NULL;
        new_bar->next_bar = NULL;
    }
    
    return new_bar;
}

/*
 * gui_bar_new_with_options: create a new bar with options
 */

struct t_gui_bar *
gui_bar_new_with_options (const char *name,
                          struct t_config_option *hidden,
                          struct t_config_option *priority,
                          struct t_config_option *type,
                          struct t_config_option *conditions,
                          struct t_config_option *position,
                          struct t_config_option *filling_top_bottom,
                          struct t_config_option *filling_left_right,
                          struct t_config_option *size,
                          struct t_config_option *size_max,
                          struct t_config_option *color_fg,
                          struct t_config_option *color_delim,
                          struct t_config_option *color_bg,
                          struct t_config_option *separator,
                          struct t_config_option *items)
{
    struct t_gui_bar *new_bar;
    struct t_gui_window *ptr_win;
    
    /* create bar */
    new_bar = gui_bar_alloc (name);
    if (new_bar)
    {
        new_bar->hidden = hidden;
        new_bar->priority = priority;
        new_bar->type = type;
        new_bar->conditions = conditions;
        if (CONFIG_STRING(conditions) && CONFIG_STRING(conditions)[0])
        {
            new_bar->conditions_array = string_explode (CONFIG_STRING(conditions),
                                                        ",", 0, 0,
                                                        &new_bar->conditions_count);
        }
        else
        {
            new_bar->conditions_count = 0;
            new_bar->conditions_array = NULL;
        }
        new_bar->position = position;
        new_bar->filling_top_bottom = filling_top_bottom;
        new_bar->filling_left_right = filling_left_right;
        new_bar->size = size;
        new_bar->size_max = size_max;
        new_bar->color_fg = color_fg;
        new_bar->color_delim = color_delim;
        new_bar->color_bg = color_bg;
        new_bar->separator = separator;
        new_bar->items = items;
        new_bar->items_count = 0;
        new_bar->items_subcount = NULL;
        new_bar->items_array = NULL;
        gui_bar_set_items_array (new_bar, CONFIG_STRING(items));
        new_bar->bar_window = NULL;
        new_bar->bar_refresh_needed = 1;
        
        /* add bar to bars list */
        gui_bar_insert (new_bar);
        
        /* add window bar */
        if (CONFIG_INTEGER(new_bar->type) == GUI_BAR_TYPE_ROOT)
        {
            /* create only one window for bar */
            gui_bar_window_new (new_bar, NULL);
            gui_window_refresh_needed = 1;
        }
        else
        {
            /* create bar window for all opened windows */
            for (ptr_win = gui_windows; ptr_win;
                 ptr_win = ptr_win->next_window)
            {
                gui_bar_window_new (new_bar, ptr_win);
            }
        }
    }
    
    return new_bar;
}

/*
 * gui_bar_new: create a new bar
 */

struct t_gui_bar *
gui_bar_new (const char *name, const char *hidden, const char *priority,
             const char *type, const char *conditions, const char *position,
             const char *filling_top_bottom, const char *filling_left_right,
             const char *size, const char *size_max,
             const char *color_fg, const char *color_delim,
             const char *color_bg, const char *separators, const char *items)
{
    struct t_config_option *option_hidden, *option_priority, *option_type;
    struct t_config_option *option_conditions, *option_position;
    struct t_config_option *option_filling_top_bottom, *option_filling_left_right;
    struct t_config_option *option_size, *option_size_max;
    struct t_config_option *option_color_fg, *option_color_delim;
    struct t_config_option *option_color_bg, *option_separator;
    struct t_config_option *option_items;
    struct t_gui_bar *new_bar;
    
    if (!name || !name[0])
        return NULL;
    
    /* it's not possible to create 2 bars with same name */
    if (gui_bar_search (name))
        return NULL;
    
    /* look for type */
    if (gui_bar_search_type (type) < 0)
        return NULL;
    
    /* look for position */
    if (gui_bar_search_position (position) < 0)
        return NULL;
    
    option_hidden = gui_bar_create_option (name, GUI_BAR_OPTION_HIDDEN,
                                           hidden);
    option_priority = gui_bar_create_option (name, GUI_BAR_OPTION_PRIORITY,
                                             priority);
    option_type = gui_bar_create_option (name, GUI_BAR_OPTION_TYPE,
                                         type);
    option_conditions = gui_bar_create_option (name, GUI_BAR_OPTION_CONDITIONS,
                                               conditions);
    option_position = gui_bar_create_option (name, GUI_BAR_OPTION_POSITION,
                                             position);
    option_filling_top_bottom = gui_bar_create_option (name, GUI_BAR_OPTION_FILLING_TOP_BOTTOM,
                                                       filling_top_bottom);
    option_filling_left_right = gui_bar_create_option (name, GUI_BAR_OPTION_FILLING_LEFT_RIGHT,
                                                       filling_left_right);
    option_size = gui_bar_create_option (name, GUI_BAR_OPTION_SIZE,
                                         size);
    option_size_max = gui_bar_create_option (name, GUI_BAR_OPTION_SIZE_MAX,
                                             size_max);
    option_color_fg = gui_bar_create_option (name, GUI_BAR_OPTION_COLOR_FG,
                                             color_fg);
    option_color_delim = gui_bar_create_option (name, GUI_BAR_OPTION_COLOR_DELIM,
                                                color_delim);
    option_color_bg = gui_bar_create_option (name, GUI_BAR_OPTION_COLOR_BG,
                                             color_bg);
    option_separator = gui_bar_create_option (name, GUI_BAR_OPTION_SEPARATOR,
                                              (config_file_string_to_boolean (separators)) ?
                                               "on" : "off");
    option_items = gui_bar_create_option (name, GUI_BAR_OPTION_ITEMS,
                                          items);
    new_bar = gui_bar_new_with_options (name, option_hidden,
                                        option_priority, option_type,
                                        option_conditions, option_position,
                                        option_filling_top_bottom,
                                        option_filling_left_right,
                                        option_size, option_size_max,
                                        option_color_fg, option_color_delim,
                                        option_color_bg, option_separator,
                                        option_items);
    if (!new_bar)
    {
        if (option_hidden)
            config_file_option_free (option_hidden);
        if (option_priority)
            config_file_option_free (option_priority);
        if (option_type)
            config_file_option_free (option_type);
        if (option_conditions)
            config_file_option_free (option_conditions);
        if (option_position)
            config_file_option_free (option_position);
        if (option_filling_top_bottom)
            config_file_option_free (option_filling_top_bottom);
        if (option_filling_left_right)
            config_file_option_free (option_filling_left_right);
        if (option_size)
            config_file_option_free (option_size);
        if (option_size_max)
            config_file_option_free (option_size_max);
        if (option_color_fg)
            config_file_option_free (option_color_fg);
        if (option_color_delim)
            config_file_option_free (option_color_delim);
        if (option_color_bg)
            config_file_option_free (option_color_bg);
        if (option_separator)
            config_file_option_free (option_separator);
        if (option_items)
            config_file_option_free (option_items);
    }
    
    return new_bar;
}

/*
 * gui_bar_use_temp_bars: use temp bars (created by reading config file)
 */

void
gui_bar_use_temp_bars ()
{
    struct t_gui_bar *ptr_temp_bar, *next_temp_bar;
    
    for (ptr_temp_bar = gui_temp_bars; ptr_temp_bar;
         ptr_temp_bar = ptr_temp_bar->next_bar)
    {
        if (!ptr_temp_bar->hidden)
            ptr_temp_bar->hidden = gui_bar_create_option (ptr_temp_bar->name,
                                                          GUI_BAR_OPTION_HIDDEN,
                                                          "0");
        if (!ptr_temp_bar->priority)
            ptr_temp_bar->priority = gui_bar_create_option (ptr_temp_bar->name,
                                                            GUI_BAR_OPTION_PRIORITY,
                                                            "0");
        if (!ptr_temp_bar->type)
            ptr_temp_bar->type = gui_bar_create_option (ptr_temp_bar->name,
                                                        GUI_BAR_OPTION_TYPE,
                                                        "0");
        if (!ptr_temp_bar->conditions)
            ptr_temp_bar->conditions = gui_bar_create_option (ptr_temp_bar->name,
                                                              GUI_BAR_OPTION_CONDITIONS,
                                                              "");
        
        if (!ptr_temp_bar->position)
            ptr_temp_bar->position = gui_bar_create_option (ptr_temp_bar->name,
                                                            GUI_BAR_OPTION_POSITION,
                                                            "top");
        
        if (!ptr_temp_bar->filling_top_bottom)
            ptr_temp_bar->filling_top_bottom = gui_bar_create_option (ptr_temp_bar->name,
                                                                      GUI_BAR_OPTION_FILLING_TOP_BOTTOM,
                                                                      "horizontal");
        
        if (!ptr_temp_bar->filling_left_right)
            ptr_temp_bar->filling_left_right = gui_bar_create_option (ptr_temp_bar->name,
                                                                      GUI_BAR_OPTION_FILLING_LEFT_RIGHT,
                                                                      "vertical");
        
        if (!ptr_temp_bar->size)
            ptr_temp_bar->size = gui_bar_create_option (ptr_temp_bar->name,
                                                        GUI_BAR_OPTION_SIZE,
                                                        "0");

        if (!ptr_temp_bar->size_max)
            ptr_temp_bar->size_max = gui_bar_create_option (ptr_temp_bar->name,
                                                            GUI_BAR_OPTION_SIZE_MAX,
                                                            "0");
        
        if (!ptr_temp_bar->color_fg)
            ptr_temp_bar->color_fg = gui_bar_create_option (ptr_temp_bar->name,
                                                            GUI_BAR_OPTION_COLOR_FG,
                                                            "default");
        
        if (!ptr_temp_bar->color_delim)
            ptr_temp_bar->color_delim = gui_bar_create_option (ptr_temp_bar->name,
                                                               GUI_BAR_OPTION_COLOR_DELIM,
                                                               "default");
        
        if (!ptr_temp_bar->color_bg)
            ptr_temp_bar->color_bg = gui_bar_create_option (ptr_temp_bar->name,
                                                            GUI_BAR_OPTION_COLOR_BG,
                                                            "default");
        
        if (!ptr_temp_bar->separator)
            ptr_temp_bar->separator = gui_bar_create_option (ptr_temp_bar->name,
                                                             GUI_BAR_OPTION_SEPARATOR,
                                                             "off");
        
        if (!ptr_temp_bar->items)
            ptr_temp_bar->items = gui_bar_create_option (ptr_temp_bar->name,
                                                         GUI_BAR_OPTION_ITEMS,
                                                         "");
        
        if (ptr_temp_bar->hidden && ptr_temp_bar->priority
            && ptr_temp_bar->type && ptr_temp_bar->conditions
            && ptr_temp_bar->position && ptr_temp_bar->filling_top_bottom
            && ptr_temp_bar->filling_left_right
            && ptr_temp_bar->size && ptr_temp_bar->size_max
            && ptr_temp_bar->color_fg && ptr_temp_bar->color_delim
            && ptr_temp_bar->color_bg && ptr_temp_bar->separator
            && ptr_temp_bar->items)
        {
            gui_bar_new_with_options (ptr_temp_bar->name,
                                      ptr_temp_bar->hidden,
                                      ptr_temp_bar->priority,
                                      ptr_temp_bar->type,
                                      ptr_temp_bar->conditions,
                                      ptr_temp_bar->position,
                                      ptr_temp_bar->filling_top_bottom,
                                      ptr_temp_bar->filling_left_right,
                                      ptr_temp_bar->size,
                                      ptr_temp_bar->size_max,
                                      ptr_temp_bar->color_fg,
                                      ptr_temp_bar->color_delim,
                                      ptr_temp_bar->color_bg,
                                      ptr_temp_bar->separator,
                                      ptr_temp_bar->items);
        }
        else
        {
            if (ptr_temp_bar->hidden)
            {
                config_file_option_free (ptr_temp_bar->hidden);
                ptr_temp_bar->hidden = NULL;
            }
            if (ptr_temp_bar->priority)
            {
                config_file_option_free (ptr_temp_bar->priority);
                ptr_temp_bar->priority = NULL;
            }
            if (ptr_temp_bar->type)
            {
                config_file_option_free (ptr_temp_bar->type);
                ptr_temp_bar->type = NULL;
            }
            if (ptr_temp_bar->conditions)
            {
                config_file_option_free (ptr_temp_bar->conditions);
                ptr_temp_bar->conditions = NULL;
            }
            if (ptr_temp_bar->position)
            {
                config_file_option_free (ptr_temp_bar->position);
                ptr_temp_bar->position = NULL;
            }
            if (ptr_temp_bar->filling_top_bottom)
            {
                config_file_option_free (ptr_temp_bar->filling_top_bottom);
                ptr_temp_bar->filling_top_bottom = NULL;
            }
            if (ptr_temp_bar->filling_left_right)
            {
                config_file_option_free (ptr_temp_bar->filling_left_right);
                ptr_temp_bar->filling_left_right = NULL;
            }
            if (ptr_temp_bar->size)
            {
                config_file_option_free (ptr_temp_bar->size);
                ptr_temp_bar->size = NULL;
            }
            if (ptr_temp_bar->size_max)
            {
                config_file_option_free (ptr_temp_bar->size_max);
                ptr_temp_bar->size_max = NULL;
            }
            if (ptr_temp_bar->color_fg)
            {
                config_file_option_free (ptr_temp_bar->color_fg);
                ptr_temp_bar->color_fg = NULL;
            }
            if (ptr_temp_bar->color_delim)
            {
                config_file_option_free (ptr_temp_bar->color_delim);
                ptr_temp_bar->color_delim = NULL;
            }
            if (ptr_temp_bar->color_bg)
            {
                config_file_option_free (ptr_temp_bar->color_bg);
                ptr_temp_bar->color_bg = NULL;
            }
            if (ptr_temp_bar->separator)
            {
                config_file_option_free (ptr_temp_bar->separator);
                ptr_temp_bar->separator = NULL;
            }
            if (ptr_temp_bar->items)
            {
                config_file_option_free (ptr_temp_bar->items);
                ptr_temp_bar->items = NULL;
            }
        }
    }
    
    /* free all temp bars */
    while (gui_temp_bars)
    {
        next_temp_bar = gui_temp_bars->next_bar;
        
        if (gui_temp_bars->name)
            free (gui_temp_bars->name);
        free (gui_temp_bars);
        
        gui_temp_bars = next_temp_bar;
    }
    last_gui_temp_bar = NULL;
}

/*
 * gui_bar_create_default_input: create default input bar if it does not exist
 */

void
gui_bar_create_default_input ()
{
    struct t_gui_bar *ptr_bar;
    int length;
    char *buf;
    
    /* search an input_text item */
    if (!gui_bar_item_used_in_a_bar (gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT], 1))
    {
        ptr_bar = gui_bar_search (GUI_BAR_DEFAULT_NAME_INPUT);
        if (ptr_bar)
        {
            /* add item "input_text" to input bar */
            length = 1;
            if (CONFIG_STRING(ptr_bar->items))
                length += strlen (CONFIG_STRING(ptr_bar->items));
            length += 1; /* "," */
            length += strlen (gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);
            buf = malloc (length);
            if (buf)
            {
                snprintf (buf, length, "%s,%s",
                          (CONFIG_STRING(ptr_bar->items)) ?
                          CONFIG_STRING(ptr_bar->items) : "",
                          gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);
                config_file_option_set (ptr_bar->items, buf, 1);
                gui_chat_printf (NULL, _("Bar \"%s\" updated"),
                                 GUI_BAR_DEFAULT_NAME_INPUT);
                gui_bar_draw (ptr_bar);
                free (buf);
            }
        }
        else
        {
            /* create input bar */
            length = 1 /* "[" */
                + strlen (gui_bar_item_names[GUI_BAR_ITEM_INPUT_PASTE])
                + 3 /* "],[" */
                + strlen (gui_bar_item_names[GUI_BAR_ITEM_INPUT_PROMPT])
                + 3 /* "],[" */
                + strlen (gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH])
                + 2 /* "]," */
                + strlen (gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT])
                + 1 /* \0 */;
            buf = malloc (length);
            if (buf)
            {
                snprintf (buf, length, "[%s],[%s],[%s],%s",
                          gui_bar_item_names[GUI_BAR_ITEM_INPUT_PASTE],
                          gui_bar_item_names[GUI_BAR_ITEM_INPUT_PROMPT],
                          gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH],
                          gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);
                if (gui_bar_new (GUI_BAR_DEFAULT_NAME_INPUT,
                                 "0",          /* hidden */
                                 "1000",       /* priority */
                                 "window",     /* type */
                                 "",           /* conditions */
                                 "bottom",     /* position */
                                 "horizontal", /* filling_top_bottom */
                                 "vertical",   /* filling_left_right */
                                 "1",          /* size */
                                 "0",          /* size_max */
                                 "default",    /* color fg */
                                 "cyan",       /* color delim */
                                 "default",    /* color bg */
                                 "0",          /* separators */
                                 buf))         /* items */
                {
                    gui_chat_printf (NULL, _("Bar \"%s\" created"),
                                     GUI_BAR_DEFAULT_NAME_INPUT);
                }
                free (buf);
            }
        }
    }
}

/*
 * gui_bar_create_default_title: create default title bar if it does not exist
 */

void
gui_bar_create_default_title ()
{
    struct t_gui_bar *ptr_bar;
    
    /* search title bar */
    ptr_bar = gui_bar_search (GUI_BAR_DEFAULT_NAME_TITLE);
    if (!ptr_bar)
    {
        /* create title bar */
        if (gui_bar_new (GUI_BAR_DEFAULT_NAME_TITLE,
                         "0",          /* hidden */
                         "500",        /* priority */
                         "window",     /* type */
                         "",           /* conditions */
                         "top",        /* position */
                         "horizontal", /* filling_top_bottom */
                         "vertical"  , /* filling_left_right */
                         "1",          /* size */
                         "0",          /* size_max */
                         "default",    /* color fg */
                         "cyan",       /* color delim */
                         "blue",       /* color bg */
                         "0",          /* separators */
                         gui_bar_item_names[GUI_BAR_ITEM_BUFFER_TITLE])) /* items */
        {
            gui_chat_printf (NULL, _("Bar \"%s\" created"),
                             GUI_BAR_DEFAULT_NAME_TITLE);
        }
    }
}

/*
 * gui_bar_create_default_status: create default status bar if it does not exist
 */

void
gui_bar_create_default_status ()
{
    struct t_gui_bar *ptr_bar;
    int length;
    char *buf;
    
    /* search status bar */
    ptr_bar = gui_bar_search (GUI_BAR_DEFAULT_NAME_STATUS);
    if (!ptr_bar)
    {
        /* create status bar */
        length = strlen (gui_bar_item_names[GUI_BAR_ITEM_TIME])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER])
            + 1 /* ":" */
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT])
            + 3 /* "lag" */
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_HOTLIST])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_COMPLETION])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_SCROLL])
            + (12 * 4) /* all items delimiters: ",:+[]()" */
            + 1;
        buf = malloc (length);
        if (buf)
        {
            snprintf (buf, length, "[%s],[%s],[%s],%s+:+%s+(%s),[lag],[%s],[%s],%s,%s",
                      gui_bar_item_names[GUI_BAR_ITEM_TIME],
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT],
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN],
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER],
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME],
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT],
                      gui_bar_item_names[GUI_BAR_ITEM_HOTLIST],
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER],
                      gui_bar_item_names[GUI_BAR_ITEM_COMPLETION],
                      gui_bar_item_names[GUI_BAR_ITEM_SCROLL]);
            if (gui_bar_new (GUI_BAR_DEFAULT_NAME_STATUS,
                             "0",          /* hidden */
                             "500",        /* priority */
                             "window",     /* type */
                             "",           /* conditions */
                             "bottom",     /* position */
                             "horizontal", /* filling_top_bottom */
                             "vertical",   /* filling_left_right */
                             "1",          /* size */
                             "0",          /* size_max */
                             "default",    /* color fg */
                             "cyan",       /* color delim */
                             "blue",       /* color bg */
                             "0",          /* separators */
                             buf))         /* items */
            {
                gui_chat_printf (NULL, _("Bar \"%s\" created"),
                                 GUI_BAR_DEFAULT_NAME_STATUS);
            }
            free (buf);
        }
    }
}

/*
 * gui_bar_create_default_nicklist: create default nicklist bar if it does not exist
 */

void
gui_bar_create_default_nicklist ()
{
    struct t_gui_bar *ptr_bar;
    
    /* search nicklist bar */
    ptr_bar = gui_bar_search (GUI_BAR_DEFAULT_NAME_NICKLIST);
    if (!ptr_bar)
    {
        /* create nicklist bar */
        if (gui_bar_new (GUI_BAR_DEFAULT_NAME_NICKLIST,
                         "0",                /* hidden */
                         "200",              /* priority */
                         "window",           /* type */
                         "nicklist",         /* conditions */
                         "right",            /* position */
                         "columns_vertical", /* filling_top_bottom */
                         "vertical",         /* filling_left_right */
                         "0",                /* size */
                         "0",                /* size_max */
                         "default",          /* color fg */
                         "cyan",             /* color delim */
                         "default",          /* color bg */
                         "1",                /* separators */
                         gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST])) /* items */
        {
            gui_chat_printf (NULL, _("Bar \"%s\" created"),
                             GUI_BAR_DEFAULT_NAME_NICKLIST);
        }
    }
}

/*
 * gui_bar_create_default: create default bars if they do not exist
 */

void
gui_bar_create_default ()
{
    gui_bar_create_default_input ();
    gui_bar_create_default_title ();
    gui_bar_create_default_status ();
    gui_bar_create_default_nicklist ();
}

/*
 * gui_bar_update: update a bar on screen
 */

void
gui_bar_update (const char *name)
{
    struct t_gui_bar *ptr_bar;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (!CONFIG_BOOLEAN(ptr_bar->hidden) && (strcmp (ptr_bar->name, name) == 0))
            gui_bar_ask_refresh (ptr_bar);
    }
}

/*
 * gui_bar_scroll: scroll a bar for a buffer
 *                 return 1 if scroll is ok, 0 if error
 */

int
gui_bar_scroll (struct t_gui_bar *bar, struct t_gui_buffer *buffer,
                const char *scroll)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    long number;
    char *str, *error;
    int length, add_x, add, percent, scroll_beginning, scroll_end;
    
    add_x = 0;
    str = NULL;
    number = 0;
    add = 0;
    percent = 0;
    scroll_beginning = 0;
    scroll_end = 0;
    
    if ((scroll[0] == 'x') || (scroll[0] == 'X'))
    {
        add_x = 1;
        scroll++;
    }
    else if ((scroll[0] == 'y') || (scroll[0] == 'Y'))
    {
        scroll++;
    }
    else
        return 0;
    
    if ((scroll[0] == 'b') || (scroll[0] == 'B'))
    {
        scroll_beginning = 1;
    }
    else if ((scroll[0] == 'e') || (scroll[0] == 'E'))
    {
        scroll_end = 1;
    }
    else
    {
        if (scroll[0] == '+')
        {
            add = 1;
            scroll++;
        }
        else if (scroll[0] == '-')
        {
            scroll++;
        }
        else
            return 0;
        
        length = strlen (scroll);
        if (length == 0)
            return 0;
    
        if (scroll[length - 1] == '%')
        {
            str = string_strndup (scroll, length - 1);
            percent = 1;
        }
        else
            str = strdup (scroll);
        if (!str)
            return 0;
        
        error = NULL;
        number = strtol (str, &error, 10);
        
        if (!error || error[0] || (number <= 0))
        {
            free (str);
            return 0;
        }
    }
    
    if (CONFIG_INTEGER(bar->type) == GUI_BAR_TYPE_ROOT)
        gui_bar_window_scroll (bar->bar_window, NULL,
                               add_x, scroll_beginning, scroll_end,
                               add, percent, number);
    else
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            if (ptr_win->buffer == buffer)
            {
                for (ptr_bar_win = ptr_win->bar_windows; ptr_bar_win;
                     ptr_bar_win = ptr_bar_win->next_bar_window)
                {
                    if (ptr_bar_win->bar == bar)
                    {
                        gui_bar_window_scroll (ptr_bar_win, ptr_win,
                                               add_x, scroll_beginning, scroll_end,
                                               add, percent, number);
                    }
                }
            }
        }
    }
    
    free (str);
    
    return 1;
}

/*
 * gui_bar_free: delete a bar
 */

void
gui_bar_free (struct t_gui_bar *bar)
{
    if (!bar)
        return;
    
    /* remove bar window(s) */
    if (bar->bar_window)
    {
        gui_bar_window_free (bar->bar_window, NULL);
        gui_window_refresh_needed = 1;
    }
    else
        gui_bar_free_bar_windows (bar);
    
    /* remove bar from bars list */
    if (bar->prev_bar)
        (bar->prev_bar)->next_bar = bar->next_bar;
    if (bar->next_bar)
        (bar->next_bar)->prev_bar = bar->prev_bar;
    if (gui_bars == bar)
        gui_bars = bar->next_bar;
    if (last_gui_bar == bar)
        last_gui_bar = bar->prev_bar;
    
    /* free data */
    if (bar->name)
        free (bar->name);
    if (bar->hidden)
        config_file_option_free (bar->hidden);
    if (bar->priority)
        config_file_option_free (bar->priority);
    if (bar->type)
        config_file_option_free (bar->type);
    if (bar->conditions)
        config_file_option_free (bar->conditions);
    if (bar->position)
        config_file_option_free (bar->position);
    if (bar->filling_top_bottom)
        config_file_option_free (bar->filling_top_bottom);
    if (bar->filling_left_right)
        config_file_option_free (bar->filling_left_right);
    if (bar->size)
        config_file_option_free (bar->size);
    if (bar->size_max)
        config_file_option_free (bar->size_max);
    if (bar->color_fg)
        config_file_option_free (bar->color_fg);
    if (bar->color_delim)
        config_file_option_free (bar->color_delim);
    if (bar->color_bg)
        config_file_option_free (bar->color_bg);
    if (bar->separator)
        config_file_option_free (bar->separator);
    if (bar->items)
        config_file_option_free (bar->items);
    if (bar->conditions_array)
        string_free_exploded (bar->conditions_array);
    gui_bar_free_items_array (bar);
    
    free (bar);
}

/*
 * gui_bar_free_all: delete all bars
 */

void
gui_bar_free_all ()
{
    while (gui_bars)
    {
        gui_bar_free (gui_bars);
    }
}

/*
 * gui_bar_free_bar_windows: free bar windows for a bar
 */

void
gui_bar_free_bar_windows (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win, *next_bar_win;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        ptr_bar_win = ptr_win->bar_windows;
        while (ptr_bar_win)
        {
            next_bar_win = ptr_bar_win->next_bar_window;
            
            if (ptr_bar_win->bar == bar)
                gui_bar_window_free (ptr_bar_win, ptr_win);
            
            ptr_bar_win = next_bar_win;
        }
    }
}

/*
 * gui_bar_add_to_infolist: add a bar in an infolist
 *                          return 1 if ok, 0 if error
 */

int
gui_bar_add_to_infolist (struct t_infolist *infolist,
                         struct t_gui_bar *bar)
{
    struct t_infolist_item *ptr_item;
    int i, j;
    char option_name[64];
    
    if (!infolist || !bar)
        return 0;
    
    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!infolist_new_var_integer (ptr_item, "hidden", CONFIG_INTEGER(bar->hidden)))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "priority", CONFIG_INTEGER(bar->priority)))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "type", CONFIG_INTEGER(bar->type)))
        return 0;
    if (!infolist_new_var_string (ptr_item, "conditions", CONFIG_STRING(bar->conditions)))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "conditions_count", bar->conditions_count))
        return 0;
    for (i = 0; i < bar->conditions_count; i++)
    {
        snprintf (option_name, sizeof (option_name),
                  "conditions_array_%05d", i + 1);
        if (!infolist_new_var_string (ptr_item, option_name,
                                      bar->conditions_array[i]))
            return 0;
    }
    if (!infolist_new_var_integer (ptr_item, "position", CONFIG_INTEGER(bar->position)))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "filling_top_bottom", CONFIG_INTEGER(bar->filling_top_bottom)))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "filling_left_right", CONFIG_INTEGER(bar->filling_left_right)))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "size", CONFIG_INTEGER(bar->size)))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "size_max", CONFIG_INTEGER(bar->size_max)))
        return 0;
    if (!infolist_new_var_string (ptr_item, "color_fg", gui_color_get_name (CONFIG_COLOR(bar->color_fg))))
        return 0;
    if (!infolist_new_var_string (ptr_item, "color_delim", gui_color_get_name (CONFIG_COLOR(bar->color_delim))))
        return 0;
    if (!infolist_new_var_string (ptr_item, "color_bg", gui_color_get_name (CONFIG_COLOR(bar->color_bg))))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "separator", CONFIG_INTEGER(bar->separator)))
        return 0;
    if (!infolist_new_var_string (ptr_item, "items", CONFIG_STRING(bar->items)))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "items_count", bar->items_count))
        return 0;
    for (i = 0; i < bar->items_count; i++)
    {
        for (j = 0; j < bar->items_subcount[i]; j++)
        {
            snprintf (option_name, sizeof (option_name),
                      "items_array_%05d_%05d", i + 1, j + 1);
            if (!infolist_new_var_string (ptr_item, option_name,
                                          bar->items_array[i][j]))
                return 0;
        }
    }
    if (!infolist_new_var_pointer (ptr_item, "bar_window", bar->bar_window))
        return 0;
    
    return 1;
}

/*
 * gui_bar_print_log: print bar infos in log (usually for crash dump)
 */

void
gui_bar_print_log ()
{
    struct t_gui_bar *ptr_bar;
    int i, j;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        log_printf ("");
        log_printf ("[bar (addr:0x%lx)]", ptr_bar);
        log_printf ("  name . . . . . . . . . : '%s'",  ptr_bar->name);
        log_printf ("  hidden . . . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->hidden));
        log_printf ("  priority . . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->priority));
        log_printf ("  type . . . . . . . . . : %d (%s)",
                    CONFIG_INTEGER(ptr_bar->type),
                    gui_bar_type_string[CONFIG_INTEGER(ptr_bar->type)]);
        log_printf ("  conditions . . . . . . : '%s'",  CONFIG_STRING(ptr_bar->conditions));
        log_printf ("  conditions_count . . . : %d",    ptr_bar->conditions_count);
        log_printf ("  conditions_array . . . : 0x%lx", ptr_bar->conditions_array);
        log_printf ("  position . . . . . . . : %d (%s)",
                    CONFIG_INTEGER(ptr_bar->position),
                    gui_bar_position_string[CONFIG_INTEGER(ptr_bar->position)]);
        log_printf ("  filling_top_bottom . . : %d (%s)",
                    CONFIG_INTEGER(ptr_bar->filling_top_bottom),
                    gui_bar_filling_string[CONFIG_INTEGER(ptr_bar->filling_top_bottom)]);
        log_printf ("  filling_left_right . . : %d (%s)",
                    CONFIG_INTEGER(ptr_bar->filling_left_right),
                    gui_bar_filling_string[CONFIG_INTEGER(ptr_bar->filling_left_right)]);
        log_printf ("  size . . . . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->size));
        log_printf ("  size_max . . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->size_max));
        log_printf ("  color_fg . . . . . . . : %d",
                    CONFIG_COLOR(ptr_bar->color_fg),
                    gui_color_get_name (CONFIG_COLOR(ptr_bar->color_fg)));
        log_printf ("  color_delim. . . . . . : %d",
                    CONFIG_COLOR(ptr_bar->color_delim),
                    gui_color_get_name (CONFIG_COLOR(ptr_bar->color_delim)));
        log_printf ("  color_bg . . . . . . . : %d",
                    CONFIG_COLOR(ptr_bar->color_bg),
                    gui_color_get_name (CONFIG_COLOR(ptr_bar->color_bg)));
        log_printf ("  separator. . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->separator));
        log_printf ("  items. . . . . . . . . : '%s'",  CONFIG_STRING(ptr_bar->items));
        log_printf ("  items_count. . . . . . : %d",    ptr_bar->items_count);
        for (i = 0; i < ptr_bar->items_count; i++)
        {
            log_printf ("    items_subcount[%03d]. : %d",
                        i, ptr_bar->items_subcount[i]);
            for (j = 0; j < ptr_bar->items_subcount[i]; j++)
            {
                log_printf ("    items_array[%03d][%03d]: '%s'",
                            i, j, ptr_bar->items_array[i][j]);
            }
        }
        log_printf ("  bar_window . . . . . . : 0x%lx", ptr_bar->bar_window);
        log_printf ("  prev_bar . . . . . . . : 0x%lx", ptr_bar->prev_bar);
        log_printf ("  next_bar . . . . . . . : 0x%lx", ptr_bar->next_bar);
        
        if (ptr_bar->bar_window)
            gui_bar_window_print_log (ptr_bar->bar_window);
    }
}
