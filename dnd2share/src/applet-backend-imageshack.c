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

#define _BSD_SOURCE
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <glib/gstdio.h>

#include "applet-struct.h"
#include "applet-dnd2share.h"
#include "applet-backend-uppix.h"

#define NB_URLS 2
static const gchar *s_UrlLabels[NB_URLS] = {"DirectLink", "Thumbnail"};


static void upload (const gchar *cFilePath)
{
	// On cree un fichier de log temporaire.
	gchar *cLogFile = g_strdup ("/tmp/dnd2share-log.XXXXXX");
	int fds = mkstemp (cLogFile);
	if (fds == -1)
	{
		g_free (cLogFile);
		return ;
	}
	close(fds);
	
	// On lance la commande d'upload.
	gchar *cCommand = NULL;
	cCommand = g_strdup_printf ("curl --connect-timeout 5 --retry 2 --limit-rate %dk http://imageshack.us -F xml=yes -F tags=Cairo-Dock -F fileupload=@'%s' -o '%s'", myConfig.iLimitRate, cFilePath, cLogFile);
	g_print ("%s\n", cCommand);
	int r = system (cCommand);
	g_free (cCommand);
	
	// On récupère l'URL dans le log :
	gchar *cURL = NULL, *cThumbnail = NULL;
	gchar *cContent = NULL;
	gsize length = 0;
	g_file_get_contents (cLogFile, &cContent, &length, NULL);
	
	gchar *str = g_strstr_len (cContent, -1, "<image_link>");
	if (str != NULL)
	{
		str += 12;  // <image_link>http://bla/bla/bla.png</image_link>
		gchar *end = g_strstr_len (str, -1, "</image_link>");
		if (end != NULL)
		{
			cURL = g_strndup (str, end - str);
		}
	}
	
	str = g_strstr_len (cContent, -1, "<thumb_link>");
	if (str != NULL)
	{
		str += 12;  // <thumb_link>http://bla/bla/bla.png</thumb_link>
		gchar *end = g_strstr_len (str, -1, "</thumb_link>");
		if (end != NULL)
		{
			cThumbnail = g_strndup (str, end - str);
		}
	}	
	
	g_free (cContent);
	g_remove (cLogFile);
	g_free (cLogFile);
	
	if (cURL == NULL)
	{
		return ;
	}
	
	// Enfin on remplit la memoire partagee avec nos URLs.
	myData.cResultUrls = g_new0 (gchar *, NB_URLS+1);
	myData.cResultUrls[0] = cURL;
	myData.cResultUrls[1] = cThumbnail;
}


void cd_dnd2share_register_imageshack_backend (void)
{
	cd_dnd2share_register_new_backend (CD_TYPE_IMAGE,
		"imageshack.us",
		NB_URLS,
		s_UrlLabels,
		0,
		upload);
}
