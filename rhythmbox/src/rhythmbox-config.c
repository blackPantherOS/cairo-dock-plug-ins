#include <string.h>

#include "rhythmbox-struct.h"
#include "rhythmbox-config.h"

#include "3dcover-draw.h"


CD_APPLET_GET_CONFIG_BEGIN
	myConfig.defaultTitle 		= CD_CONFIG_GET_STRING ("Icon", "name");
	
	myConfig.enableDialogs 		= CD_CONFIG_GET_BOOLEAN ("Configuration", "enable_dialogs");
	myConfig.enableCover 		= CD_CONFIG_GET_BOOLEAN ("Configuration", "enable_cover");
	myConfig.timeDialogs 		= CD_CONFIG_GET_DOUBLE_WITH_DEFAULT ("Configuration", "time_dialogs", 3000);
	myConfig.changeAnimation 	= CD_CONFIG_GET_STRING ("Configuration", "change animation");
	myConfig.quickInfoType 		= CD_CONFIG_GET_INTEGER_WITH_DEFAULT ("Configuration", "quick-info_type", MY_APPLET_TIME_ELAPSED);
	myConfig.bStealTaskBarIcon = CD_CONFIG_GET_BOOLEAN ("Configuration", "inhibate appli");
	
	myConfig.cUserImage[PLAYER_NONE] 			= CD_CONFIG_GET_STRING ("Configuration", "default icon");
	myConfig.cUserImage[PLAYER_PLAYING] 		= CD_CONFIG_GET_STRING ("Configuration", "play icon");
	myConfig.cUserImage[PLAYER_PAUSED] 		= CD_CONFIG_GET_STRING ("Configuration", "pause icon");
	myConfig.cUserImage[PLAYER_STOPPED] 		= CD_CONFIG_GET_STRING ("Configuration", "stop icon");
	myConfig.cUserImage[PLAYER_BROKEN] 		= CD_CONFIG_GET_STRING ("Configuration", "broken icon");
	
	myConfig.extendedDesklet = CD_CONFIG_GET_BOOLEAN ("Configuration", "3D desklet");
	
	myConfig.iDeskletWidth	= CD_CONFIG_GET_INTEGER ("Desklet", "width");
	myConfig.iDeskletHeight	= CD_CONFIG_GET_INTEGER ("Desklet", "height");
	
	
	//\_______________ On on recupere le theme choisi.
	myConfig.cThemePath = CD_CONFIG_GET_THEME_PATH ("Configuration", "theme", "themes", "cd_box_3d");
	cd_opengl_load_external_conf_theme_values (myApplet);
	
CD_APPLET_GET_CONFIG_END


CD_APPLET_RESET_CONFIG_BEGIN
	g_free (myConfig.defaultTitle);
	g_free (myConfig.changeAnimation);
	
	int i;
	for (i = 0; i < PLAYER_NB_STATUS; i ++)
		g_free (myConfig.cUserImage[i]);
	
	g_free (myConfig.cThemePath);
	g_free (myData.cThemeFrame);
	g_free (myData.cThemeReflect);
	
CD_APPLET_RESET_CONFIG_END


CD_APPLET_RESET_DATA_BEGIN
	int i;
	for (i = 0; i < PLAYER_NB_STATUS; i ++)
	{
		if (myData.pSurfaces[i] != NULL)
			cairo_surface_destroy (myData.pSurfaces[i]);
	}
	
	cairo_surface_destroy (myData.pCover);
	
	g_free (myData.playing_uri);
	g_free (myData.playing_artist);
	g_free (myData.playing_album);
	g_free (myData.playing_title);
	
	glDeleteTextures (1, &myData.TextureFrame);
	glDeleteTextures (1, &myData.TextureCover);
	glDeleteTextures (1, &myData.TextureReflect);
	glDeleteTextures (1, &myData.TextureName);
	glDeleteTextures (1, &myData.TextureButton1);
	glDeleteTextures (1, &myData.TextureButton2);
	glDeleteTextures (1, &myData.TextureButton3);
	glDeleteTextures (1, &myData.TextureButton4);
	glDeleteTextures (1, &myData.TextureOsdPlay);
	glDeleteTextures (1, &myData.TextureOsdPause);
	glDeleteTextures (1, &myData.TextureOsdPrev);
	glDeleteTextures (1, &myData.TextureOsdNext);
	glDeleteTextures (1, &myData.TextureOsdHome);
	
CD_APPLET_RESET_DATA_END
