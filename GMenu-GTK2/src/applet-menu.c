/**
* This file is a part of the Cairo-Dock project
*
* Copyright : (C) see the 'copyright' file.
* E-mail    : see the 'copyright' file.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 3
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <cairo-dock.h>

#include "applet-struct.h"
#include "applet-menu-callbacks.h"
#include "applet-util.h"
#include "applet-menu.h"


GtkWidget * add_menu_separator (GtkWidget *menu)
{
	GtkWidget *menuitem;
	
	menuitem = gtk_separator_menu_item_new ();
	gtk_widget_set_sensitive (menuitem, FALSE);
	gtk_widget_show (menuitem);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

	return menuitem;
}

/* REM: if this function is used in a thread (myData.bLaunchInThread == TRUE)
 *  submenu_to_display will be launched there but no notification will be send
 *  after having created this fake menu
 */
GtkWidget * create_fake_menu (GMenuTreeDirectory *directory)
{	
	GtkWidget *menu;
	guint      idle_id;
	
	menu = create_empty_menu ();

	g_object_set_data_full (G_OBJECT (menu),
				"panel-menu-tree-directory",
				gmenu_tree_item_ref (directory),
				(GDestroyNotify) gmenu_tree_item_unref);
	
	g_object_set_data (G_OBJECT (menu),
			   "panel-menu-needs-loading",
			   GUINT_TO_POINTER (TRUE));

	g_signal_connect (menu, "show",
			  G_CALLBACK (submenu_to_display), NULL);

	if (! myData.bLoadInThread)
	{
		idle_id = g_idle_add_full (G_PRIORITY_LOW,
					submenu_to_display_in_idle,
					menu,
					NULL);
		g_object_set_data_full (G_OBJECT (menu),
					"panel-menu-idle-id",
					GUINT_TO_POINTER (idle_id),
					remove_submenu_to_display_idle);
	}
	else
		submenu_to_display (menu);

	return menu;
}

void image_menu_destroy (GtkWidget *image, gpointer *data)
{
	myData.image_menu_items = g_slist_remove (myData.image_menu_items, image);
	if (myConfig.bLoadIconsAtStartup && ! myData.bIconsLoaded && myData.pPreloadedImagesList && data)
	{ // we want to preload icon, the task has not been launched, the list is not empty and we receive data
		myData.pPreloadedImagesList = g_list_remove (myData.pPreloadedImagesList, data);
		g_free (data);
	}
}


void reload_image_menu_items (void)
{
	GSList *l;

	for (l = myData.image_menu_items; l; l = l->next) {
		GtkWidget *image = l->data;
		gboolean   is_mapped;
      
		///is_mapped = GTK_WIDGET_MAPPED (image);
		is_mapped = gtk_widget_get_mapped (image);
		
		if (is_mapped)
			gtk_widget_unmap (image);

		gtk_image_set_from_pixbuf (GTK_IMAGE (image), NULL);
    
		if (is_mapped)
			gtk_widget_map (image);

	}
}

static void
remove_pixmap_from_loaded (gpointer data, GObject *where_the_object_was)
{
	char *key = data;

	if (myData.loaded_icons != NULL)
		g_hash_table_remove (myData.loaded_icons, key);

	g_free (key);
}
GdkPixbuf * panel_make_menu_icon (GtkIconTheme *icon_theme,
		      const char   *icon,
		      const char   *fallback,
		      int           size,
		      gboolean     *long_operation)
{
	GdkPixbuf *pb;
	char *file, *key;
	gboolean loaded;

	g_return_val_if_fail (size > 0, NULL);

	file = NULL;
	if (icon != NULL)
		file = panel_find_icon (icon_theme, icon, size);
	if (file == NULL && fallback != NULL)
		file = panel_find_icon (icon_theme, fallback, size);

	if (file == NULL)
		return NULL;

	if (long_operation != NULL)
		*long_operation = TRUE;

	pb = NULL;

	loaded = FALSE;

	key = g_strdup_printf ("%d:%s", size, file);

	if (myData.loaded_icons != NULL &&
	    (pb = g_hash_table_lookup (myData.loaded_icons, key)) != NULL) {
		if (pb != NULL)
			g_object_ref (G_OBJECT (pb));
	}

	if (pb == NULL) {
		pb = gdk_pixbuf_new_from_file (file, NULL);
		if (pb) {
			gint width, height;

			width = gdk_pixbuf_get_width (pb);
			height = gdk_pixbuf_get_height (pb);
			
			/* if we want 24 and we get 22, do nothing;
			 * else scale */
			if (!(size - 2 <= width && width <= size &&
                              size - 2 <= height && height <= size)) {
				GdkPixbuf *tmp;

				tmp = gdk_pixbuf_scale_simple (pb, size, size,
							       GDK_INTERP_BILINEAR);

				g_object_unref (pb);
				pb = tmp;
			}
		}
				
		/* add icon to the hash table so we don't load it again */
		loaded = TRUE;
	}

	if (pb == NULL) {
		g_free (file);
		g_free (key);
		return NULL;
	}

	if (loaded &&
	    (gdk_pixbuf_get_width (pb) != size &&
	     gdk_pixbuf_get_height (pb) != size)) {
		GdkPixbuf *pb2;
		int        dest_width;
		int        dest_height;
		int        width;
		int        height;

		width  = gdk_pixbuf_get_width (pb);
		height = gdk_pixbuf_get_height (pb);

		if (height > width) {
			dest_width  = (size * width) / height;
			dest_height = size;
		} else {
			dest_width  = size;
			dest_height = (size * height) / width;
		}

		pb2 = gdk_pixbuf_scale_simple (pb, dest_width, dest_height,
					       GDK_INTERP_BILINEAR);
		g_object_unref (G_OBJECT (pb));
		pb = pb2;
	}

	if (loaded) {
		if (myData.loaded_icons == NULL)
			myData.loaded_icons = g_hash_table_new_full
				(g_str_hash, g_str_equal,
				 (GDestroyNotify) g_free,
				 (GDestroyNotify) g_object_unref);
		g_hash_table_replace (myData.loaded_icons,
				      g_strdup (key),
				      g_object_ref (G_OBJECT (pb)));
		g_object_weak_ref (G_OBJECT (pb),
				   (GWeakNotify) remove_pixmap_from_loaded,
				   g_strdup (key));
	} else {
		/* we didn't load from disk */
		if (long_operation != NULL)
			*long_operation = FALSE;
	}

	g_free (file);
	g_free (key);

	return pb;
}

void panel_load_menu_image_deferred (GtkWidget   *image_menu_item,
				GtkIconSize  icon_size,
				///const char  *stock_id,
				///GIcon       *gicon,
				const char  *image_filename,
				const char  *fallback_image_filename)
{
	IconToLoad *icon;
	GtkWidget *image;
	int        icon_height = myData.iPanelDefaultMenuIconSize;

	icon = g_new0 (IconToLoad, 1);

	gtk_icon_size_lookup (icon_size, NULL, &icon_height);

	image = gtk_image_new ();
	///image->requisition.width  = icon_height;
	///image->requisition.height = icon_height;
	gtk_widget_set_size_request (image, icon_height, icon_height);
	
	/* this takes over the floating ref */
	icon->pixmap = g_object_ref (G_OBJECT (image));
	g_object_ref_sink (G_OBJECT (image));

	/**icon->stock_id       = stock_id;
	if (gicon)
		icon->gicon  = g_object_ref (gicon);
	else
		icon->gicon  = NULL;*/
	icon->image          = g_strdup (image_filename);
	icon->fallback_image = g_strdup (fallback_image_filename);
	icon->icon_size      = icon_size;

	/**g_object_set_data_full (G_OBJECT (image_menu_item),
				"Panel:Image",
				g_object_ref (image),
				(GDestroyNotify) g_object_unref);*/

	g_signal_connect_data (image, "map",
			       G_CALLBACK (image_menu_shown), icon,
			       (GClosureNotify) icon_to_load_free, 0);

	// pre-load all icons
	gpointer *data = NULL;
	if (myConfig.bLoadIconsAtStartup && ! myData.bIconsLoaded)
	{
		data = g_new0 (gpointer, 2);
		data[0] = image;
		data[1] = icon;
		myData.pPreloadedImagesList = g_list_append (myData.pPreloadedImagesList, data);
	}
 
	_gtk_image_menu_item_set_image (
		GTK_IMAGE_MENU_ITEM (image_menu_item), image);

	gtk_widget_show (image);

	g_signal_connect (image, "destroy",
			  G_CALLBACK (image_menu_destroy), data);

	myData.image_menu_items = g_slist_prepend (myData.image_menu_items, image);
}
GtkWidget * create_submenu_entry (GtkWidget          *menu,
		      GMenuTreeDirectory *directory)
{
	GtkWidget *menuitem;

	menuitem = gtk_image_menu_item_new ();

	panel_load_menu_image_deferred (menuitem,
					32, //panel_menu_icon_get_size (),
					///NULL, NULL,
					gmenu_tree_directory_get_icon (directory),
					PANEL_ICON_FOLDER);

	setup_menuitem (menuitem,
			32, //panel_menu_icon_get_size (),
			///NULL,
			gmenu_tree_directory_get_name (directory));

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

	gtk_widget_show (menuitem);

	return menuitem;
}

void create_submenu (GtkWidget          *menu,
		GMenuTreeDirectory *directory,
		GMenuTreeDirectory *alias_directory)
{
	GtkWidget *menuitem;
	GtkWidget *submenu;

	if (alias_directory)
		menuitem = create_submenu_entry (menu, alias_directory);
	else
		menuitem = create_submenu_entry (menu, directory);
	
	submenu = create_fake_menu (directory);
	

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);
}

void create_header (GtkWidget       *menu,
	       GMenuTreeHeader *header)
{
	GMenuTreeDirectory *directory;
	GtkWidget          *menuitem;

	directory = gmenu_tree_header_get_directory (header);
	menuitem = create_submenu_entry (menu, directory);
	gmenu_tree_item_unref (directory);

	g_object_set_data_full (G_OBJECT (menuitem),
				"panel-gmenu-tree.header",
				gmenu_tree_item_ref (header),
				(GDestroyNotify) gmenu_tree_item_unref);

	g_signal_connect (menuitem, "activate",
			  G_CALLBACK (gtk_false), NULL);
}

void create_menuitem (GtkWidget          *menu,
		 GMenuTreeEntry     *entry,
		 GMenuTreeDirectory *alias_directory)
{
	GtkWidget  *menuitem;
	
	menuitem = gtk_image_menu_item_new ();

	g_object_set_data_full (G_OBJECT (menuitem),
				"panel-menu-tree-entry",
				gmenu_tree_item_ref (entry),
				(GDestroyNotify) gmenu_tree_item_unref);

	if (alias_directory)
		//FIXME: we should probably use this data when we do dnd or
		//context menu for this menu item
		g_object_set_data_full (G_OBJECT (menuitem),
					"panel-menu-tree-alias-directory",
					gmenu_tree_item_ref (alias_directory),
					(GDestroyNotify) gmenu_tree_item_unref);

	panel_load_menu_image_deferred (menuitem,
					myData.iPanelDefaultMenuIconSize, //panel_menu_icon_get_size (),
					///NULL, NULL,
					alias_directory ? gmenu_tree_directory_get_icon (alias_directory) :
							  gmenu_tree_entry_get_icon (entry),
					NULL);

	setup_menuitem (menuitem,
			myData.iPanelDefaultMenuIconSize, //panel_menu_icon_get_size (),
			///NULL,
			alias_directory ? gmenu_tree_directory_get_name (alias_directory) :
					  gmenu_tree_entry_get_name (entry));

	if ((alias_directory &&
	     gmenu_tree_directory_get_comment (alias_directory)) ||
	    (!alias_directory &&
	     gmenu_tree_entry_get_comment (entry)))
		panel_util_set_tooltip_text (menuitem,
					     alias_directory ?
						gmenu_tree_directory_get_comment (alias_directory) :
						gmenu_tree_entry_get_comment (entry));

	/*g_signal_connect_after (menuitem, "button_press_event",
				G_CALLBACK (menuitem_button_press_event), NULL);*/

	//if (!panel_lockdown_get_locked_down ()) {
	{
		static GtkTargetEntry menu_item_targets[] = {
			{ (gchar*)"text/uri-list", 0, 0 }
		};

		gtk_drag_source_set (menuitem,
				     GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
				     menu_item_targets, 1,
				     GDK_ACTION_COPY);

		if (gmenu_tree_entry_get_icon (entry) != NULL) {
			const char *icon;
			char       *icon_no_ext;

			icon = gmenu_tree_entry_get_icon (entry);
			if (!g_path_is_absolute (icon)) {
				icon_no_ext = panel_util_icon_remove_extension (icon);
				gtk_drag_source_set_icon_name (menuitem,
							       icon_no_ext);
				g_free (icon_no_ext);
			}
		}

		///g_signal_connect (G_OBJECT (menuitem), "drag_begin",
		///		  G_CALLBACK (drag_begin_menu_cb), NULL);
		g_signal_connect (menuitem, "drag_data_get",
				  G_CALLBACK (drag_data_get_menu_cb), entry);
		///g_signal_connect (menuitem, "drag_end",
		///		  G_CALLBACK (drag_end_menu_cb), NULL);
	}

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

	g_signal_connect (menuitem, "activate",
			  G_CALLBACK (activate_app_def), entry);

	gtk_widget_show (menuitem);
}

void create_menuitem_from_alias (GtkWidget      *menu,
			    GMenuTreeAlias *alias)
{
	GMenuTreeItem *aliased_item;

	aliased_item = gmenu_tree_alias_get_item (alias);

	switch (gmenu_tree_item_get_type (aliased_item)) {
	case GMENU_TREE_ITEM_DIRECTORY:
		create_submenu (menu,
				GMENU_TREE_DIRECTORY (aliased_item),
				gmenu_tree_alias_get_directory (alias));
		break;

	case GMENU_TREE_ITEM_ENTRY:
		create_menuitem (menu,
				 GMENU_TREE_ENTRY (aliased_item),
				 gmenu_tree_alias_get_directory (alias));
		break;

	default:
		break;
	}

	gmenu_tree_item_unref (aliased_item);
}


/**static void
image_menuitem_size_request (GtkWidget      *menuitem,
			     GtkRequisition *requisition,
			     gpointer        data)
{
	GtkIconSize icon_size = (GtkIconSize) GPOINTER_TO_INT (data);
	int         icon_height;
	int         req_height;

	if (!gtk_icon_size_lookup (icon_size, NULL, &icon_height))
		return;

	// If we don't have a pixmap for this menuitem
	// at least make sure its the same height as
	// the rest.
	// This is a bit ugly, since we should keep this in sync with what's in
	// gtk_menu_item_size_request()
	req_height = icon_height;
	req_height += (GTK_CONTAINER (menuitem)->border_width +
		       menuitem->style->thickness) * 2;
	requisition->height = MAX (requisition->height, req_height);
}*/
void setup_menuitem (GtkWidget   *menuitem,
		GtkIconSize  icon_size,
		///GtkWidget   *image,
		const char  *title)
			       
{
	GtkWidget *label;
	char      *_title;

	/* this creates a label with an invisible mnemonic */
	label = g_object_new (GTK_TYPE_ACCEL_LABEL, NULL);
	_title = menu_escape_underscores_and_prepend (title);
	gtk_label_set_text_with_mnemonic (GTK_LABEL (label), _title);
	g_free (_title);

	gtk_label_set_pattern (GTK_LABEL (label), "");

	gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), menuitem);

	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);

	gtk_container_add (GTK_CONTAINER (menuitem), label);

	/**if (image) {
		g_object_set_data_full (G_OBJECT (menuitem),
					"Panel:Image",
					g_object_ref (image),
					(GDestroyNotify) g_object_unref);
		gtk_widget_show (image);
		_gtk_image_menu_item_set_image (
			GTK_IMAGE_MENU_ITEM (menuitem), image);
	} else if (icon_size != GTK_ICON_SIZE_INVALID)
		g_signal_connect (menuitem, "size_request",
				  G_CALLBACK (image_menuitem_size_request),
				  GINT_TO_POINTER (icon_size));
	*/
	gtk_widget_show (menuitem);
}

GtkWidget * populate_menu_from_directory (GtkWidget          *menu,
			      GMenuTreeDirectory *directory)
{	
	GSList   *l;
	GSList   *items;
	gboolean  add_separator;

	///add_separator = (GTK_MENU_SHELL (menu)->children != NULL);
	GList *children = gtk_container_get_children (GTK_CONTAINER (menu));
	add_separator = (children != NULL);
	g_list_free (children);  // not very optimized ...	
	
	items = gmenu_tree_directory_get_contents (directory);

	for (l = items; l; l = l->next) {
		GMenuTreeItem *item = l->data;

		if (add_separator ||
		    gmenu_tree_item_get_type (item) == GMENU_TREE_ITEM_SEPARATOR) {
			add_menu_separator (menu);
			add_separator = FALSE;
		}

		switch (gmenu_tree_item_get_type (item)) {
		case GMENU_TREE_ITEM_DIRECTORY:
			create_submenu (menu, GMENU_TREE_DIRECTORY (item), NULL);
			break;

		case GMENU_TREE_ITEM_ENTRY:
			create_menuitem (menu, GMENU_TREE_ENTRY (item), NULL);
			break;

		case GMENU_TREE_ITEM_SEPARATOR :
			/* already added */
			break;

		case GMENU_TREE_ITEM_ALIAS:
			create_menuitem_from_alias (menu, GMENU_TREE_ALIAS (item));
			break;

		case GMENU_TREE_ITEM_HEADER:
			create_header (menu, GMENU_TREE_HEADER (item));
			break;

		default:
			break;
		}

		gmenu_tree_item_unref (item);
	}

	g_slist_free (items);

	return menu;
}



/**void icon_theme_changed (GtkIconTheme *icon_theme,
		    gpointer      data)
{
	reload_image_menu_items ();
}

static inline GtkWidget * panel_create_menu (void)
{
	GtkWidget       *retval;
	static gboolean  registered_icon_theme_changer = FALSE;

	if (!registered_icon_theme_changer) {
		registered_icon_theme_changer = TRUE;

		g_signal_connect (gtk_icon_theme_get_default (), "changed",
				  G_CALLBACK (icon_theme_changed), NULL);
	}
	
	retval = gtk_menu_new ();
	
	return retval;
}*/
GtkWidget * create_empty_menu (void)
{
	GtkWidget *retval;

	///retval = panel_create_menu ();
	retval = gtk_menu_new ();
	
	return retval;
}

static void _on_remove_tree (GMenuTree  *tree)
{
	cd_message ("%s (%x)", __func__, tree);
	//gmenu_tree_unref (tree);
}

/* REM: if this function is used in a thread (myData.bLaunchInThread == TRUE)
 *  submenu_to_display should be launched after having set data to the menu
 *  for keys "panel-menu-append-callback*"
 */
GtkWidget * create_applications_menu (const char *menu_file,
			  const char *menu_path, GtkWidget *parent_menu)
{
	GMenuTree *tree;
	GtkWidget *menu;
	guint      idle_id;

	menu = (parent_menu ? parent_menu : create_empty_menu ());
	
	cd_message ("%s (%s)", __func__, menu_file);
	tree = gmenu_tree_lookup (menu_file, GMENU_TREE_FLAGS_NONE);
	cd_debug (" tree : %x", tree);

	g_object_set_data_full (G_OBJECT (menu),
				"panel-menu-tree",
				gmenu_tree_ref (tree),
				(GDestroyNotify) _on_remove_tree);

	g_object_set_data_full (G_OBJECT (menu),
				"panel-menu-tree-path",
				g_strdup (menu_path ? menu_path : "/"),
				(GDestroyNotify) g_free);
	
	g_object_set_data (G_OBJECT (menu),
			   "panel-menu-needs-loading",
			   GUINT_TO_POINTER (TRUE));
	
	// load the menu in idle, and force the loading if it's shown before.
	g_signal_connect (menu, "show",
			  G_CALLBACK (submenu_to_display), NULL);

	if (! myData.bLoadInThread)
	{
		idle_id = g_idle_add_full (G_PRIORITY_LOW,
					submenu_to_display_in_idle,
					menu,
					NULL);
		g_object_set_data_full (G_OBJECT (menu),
					"panel-menu-idle-id",
					GUINT_TO_POINTER (idle_id),
					remove_submenu_to_display_idle); // => g_source_remove (idle_id);
	}
	// else: submenu_to_display should be launched after...

	gmenu_tree_add_monitor (tree,
			       (GMenuTreeChangedFunc) handle_gmenu_tree_changed,
			       menu);
	g_signal_connect (menu, "destroy",
			  G_CALLBACK (remove_gmenu_tree_monitor), tree);

	gmenu_tree_unref (tree);

	return menu;
}

// $XDG_CONFIG_DIRS => /etc/xdg/xdg-cairo-dock:/etc/xdg
// http://developer.gnome.org/menu-spec/
gchar ** cd_gmenu_get_xdg_menu_dirs (void)
{
	const gchar *cMenuPrefix = g_getenv ("XDG_CONFIG_DIRS");
	if (! cMenuPrefix || *cMenuPrefix == '\0')
		cMenuPrefix = "/etc/xdg/menus";

	return g_strsplit (cMenuPrefix, ":", 0);
}

// check if the file exists and if yes, *cMenuName is created
gboolean _check_file_exists (const gchar *cDir, const gchar *cPrefix, gchar **cMenuName)
{
	gchar *cMenuFilePathWithPrefix = g_strdup_printf ("%s/menus/%sapplications.menu", cDir, cPrefix);

	gboolean bFileExists = g_file_test (cMenuFilePathWithPrefix, G_FILE_TEST_EXISTS);
	if (bFileExists)
		*cMenuName = g_strdup_printf ("%sapplications.menu", cPrefix);

	cd_debug ("Check: %s: %d", cMenuFilePathWithPrefix, bFileExists);
	g_free (cMenuFilePathWithPrefix);
	return bFileExists;
}

static const gchar *cPrefixNames[] = {"", "gnome-", "kde-", "kde4-", "xfce-", "lxde-", NULL};

GtkWidget * create_main_menu (CairoDockModuleInstance *myApplet)
{
	GtkWidget *main_menu;

	gchar *cMenuFileName = NULL, *cXdgMenuPath = NULL;
	const gchar *cMenuPrefix = g_getenv ("XDG_MENU_PREFIX"); // e.g. on xfce, it contains "xfce-", nothing on gnome
	gchar **cXdgPath = cd_gmenu_get_xdg_menu_dirs ();

	int i;
	for (i = 0; cXdgPath[i] != NULL; i++)
	{
		g_free (cXdgMenuPath);
		cXdgMenuPath = g_strdup_printf ("%s/menus", cXdgPath[i]);
		if (! g_file_test (cXdgMenuPath, G_FILE_TEST_IS_DIR)) // cXdgPath can contain an invalid dir
			continue;

		// this test should be the good one: with or without the prefix
		if (_check_file_exists (cXdgPath[i], cMenuPrefix ? cMenuPrefix : "", &cMenuFileName))
			break;

		// let's check with common prefixes
		for (int iPrefix = 0; cPrefixNames[iPrefix] != NULL; iPrefix++)
		{
			if (_check_file_exists (cXdgPath[i], cPrefixNames[iPrefix], &cMenuFileName))
				break;
		}

		if (cMenuFileName == NULL) // let's check any *-applications.menu
		{
			const gchar *cFileName;
			GDir *dir = g_dir_open (cXdgPath[i], 0, NULL);
			if (dir)
			{
				while ((cFileName = g_dir_read_name (dir)))
				{
					if (g_str_has_suffix (cFileName, "-applications.menu"))
					{
						cMenuFileName = g_strdup (cFileName);
						break;
					}
				}
				g_dir_close (dir);
				if (cMenuFileName != NULL)
					break;
			}
		}
	}

	cd_debug ("Menu: Found %s in %s (%s)", cMenuFileName, cXdgPath[i], cXdgMenuPath);

	if (cMenuFileName == NULL) // arf
		cMenuFileName = g_strdup ("applications.menu");

	main_menu = create_applications_menu (cMenuFileName, NULL, NULL);

	g_object_set_data (G_OBJECT (main_menu),
		"panel-menu-append-callback",
		main_menu_append);
	g_object_set_data (G_OBJECT (main_menu),
		"panel-menu-append-callback-data",
		myApplet);

	if (myData.bLoadInThread) // load submenu in a thread
		submenu_to_display (main_menu);

	g_strfreev (cXdgPath);
	g_free (cMenuFileName);
	g_free (cXdgMenuPath);

	return main_menu;
}