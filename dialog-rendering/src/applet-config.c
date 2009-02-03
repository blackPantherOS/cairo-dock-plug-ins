/******************************************************************************

This file is a part of the cairo-dock program, 
released under the terms of the GNU General Public License.

Written by Fabrice Rey (for any bug report, please mail me to fabounet@users.berlios.de)

******************************************************************************/
#include <math.h>
#include <cairo-dock.h>

#include "applet-struct.h"
#include "applet-config.h"


CD_APPLET_GET_CONFIG_BEGIN
	myConfig.iComicsRadius = CD_CONFIG_GET_INTEGER ("Comics", "corner");
	myConfig.iComicsLineWidth = CD_CONFIG_GET_INTEGER ("Comics", "border");
	CD_CONFIG_GET_COLOR ("Comics", "line color", &myConfig.fComicsLineColor);
	
	myConfig.iModernRadius = CD_CONFIG_GET_INTEGER ("Modern", "corner");
	myConfig.iModernLineWidth = CD_CONFIG_GET_INTEGER ("Modern", "border");
	CD_CONFIG_GET_COLOR ("Modern", "line color", &myConfig.fModernLineColor);
	myConfig.iModernLineSpacing = CD_CONFIG_GET_INTEGER ("Modern", "line space");
	
	myConfig.iPlaneRadius = CD_CONFIG_GET_INTEGER ("3D plane", "corner");
	myConfig.iPlaneLineWidth = CD_CONFIG_GET_INTEGER ("3D plane", "border");
	CD_CONFIG_GET_COLOR ("3D plane", "line color", &myConfig.fPlaneLineColor);
	CD_CONFIG_GET_COLOR ("3D plane", "plane color", &myConfig.fPlaneColor);
	
	myConfig.iTooltipRadius = CD_CONFIG_GET_INTEGER ("Tooltip", "corner");
	myConfig.iTooltipLineWidth = CD_CONFIG_GET_INTEGER ("Tooltip", "border");
	CD_CONFIG_GET_COLOR ("Tooltip", "line color", &myConfig.fTooltipLineColor);
	CD_CONFIG_GET_COLOR ("Tooltip", "margin color", &myConfig.fTooltipMarginColor);
CD_APPLET_GET_CONFIG_END


CD_APPLET_RESET_CONFIG_BEGIN
	
CD_APPLET_RESET_CONFIG_END


CD_APPLET_RESET_DATA_BEGIN
	
CD_APPLET_RESET_DATA_END
