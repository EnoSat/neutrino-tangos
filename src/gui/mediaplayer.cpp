/*
	Mediaplayer selection menu - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2011 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

	License: GPL

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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "mediaplayer.h"

#include <global.h>
#include <neutrino.h>
#include <neutrino_menue.h>
#include <neutrinoMessages.h>

#include <gui/infoclock.h>
#include <gui/movieplayer.h>
#if ENABLE_UPNP
#include <gui/upnpbrowser.h>
#endif
#include <gui/nfs.h>

#include <gui/widget/icons.h>

#include <driver/screen_max.h>

#include <system/debug.h>
#include <libdvbapi/video.h>
extern cVideo * videoDecoder;


CMediaPlayerMenu::CMediaPlayerMenu()
{
	width = 40;
	
}

CMediaPlayerMenu* CMediaPlayerMenu::getInstance()
{
	static CMediaPlayerMenu* mpm = NULL;

	if(!mpm) {
		mpm = new CMediaPlayerMenu();
		printf("[neutrino] mediaplayer menu instance created\n");
	}
	return mpm;
}

CMediaPlayerMenu::~CMediaPlayerMenu()
{
}

int CMediaPlayerMenu::exec(CMenuTarget* parent, const std::string &actionKey)
{
	if (parent)
		parent->hide();
	
	if (actionKey == "moviebrowser")
	{
		CInfoClock::getInstance()->enableInfoClock(false);
		int mode = CNeutrinoApp::getInstance()->getMode();
		if( mode == NeutrinoModes::mode_radio )
			CFrameBuffer::getInstance()->stopFrame();
		int res = CMoviePlayerGui::getInstance().exec(NULL, "tsmoviebrowser");
		if( mode == NeutrinoModes::mode_radio )
			CFrameBuffer::getInstance()->showFrame("radiomode.jpg");
		CInfoClock::getInstance()->enableInfoClock(true);
		return res;
	}
	
	int res = initMenuMedia();
	
	return res;
}

//show selectable mediaplayer items
int CMediaPlayerMenu::initMenuMedia(CMenuWidget *m, CPersonalizeGui *p)
{	
	CPersonalizeGui *personalize = p;
	CMenuWidget *multimedia_menu = m;
	
	bool show = (personalize == NULL || multimedia_menu == NULL);

	if (personalize == NULL)
		 personalize = new CPersonalizeGui();
	
	if (multimedia_menu == NULL)
		 multimedia_menu = new CMenuWidget(LOCALE_MAINMENU_MEDIA, NEUTRINO_ICON_MULTIMEDIA, width, MN_WIDGET_ID_MEDIA);

	personalize->addWidget(multimedia_menu);
	personalize->addIntroItems(multimedia_menu);
	
	bool enabled = !CMoviePlayerGui::getInstance().Playing();

	personalize->addSeparator(*multimedia_menu, LOCALE_MAINMENU_MOVIEPLAYER, true);

	//moviebrowser
	CMenuForwarder *fw_mbrowser = new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, this, "moviebrowser", CRCInput::RC_red);
	fw_mbrowser->setHint(NEUTRINO_ICON_HINT_MB, LOCALE_MENU_HINT_MB);
	personalize->addItem(multimedia_menu, fw_mbrowser, &g_settings.personalize[SNeutrinoSettings::P_MPLAYER_MBROWSER]);

	//fileplayback
	CMenuForwarder *fw_fileplay = new CMenuForwarder(LOCALE_MOVIEPLAYER_FILEPLAYBACK, true, NULL, &CMoviePlayerGui::getInstance(), "fileplayback", CRCInput::RC_green);
	fw_fileplay->setHint(NEUTRINO_ICON_HINT_FILEPLAY, LOCALE_MENU_HINT_FILEPLAY);
	personalize->addItem(multimedia_menu, fw_fileplay, &g_settings.personalize[SNeutrinoSettings::P_MPLAYER_FILEPLAY]);

#ifdef ENABLE_GUI_MOUNT
	if (g_settings.personalize[SNeutrinoSettings::P_MPLAYER_GUI_MOUNT])
		personalize->addSeparator(*multimedia_menu, LOCALE_NETWORKMENU_MOUNT, true); 

	CMenuForwarder * mf_mount = new CMenuForwarder(LOCALE_NFS_MOUNT , true, NULL, new CNFSMountGui(), NULL, CRCInput::RC_yellow);
	mf_mount->setHint("", LOCALE_MENU_HINT_NET_NFS_MOUNT);
	personalize->addItem(multimedia_menu, mf_mount, &g_settings.personalize[SNeutrinoSettings::P_MPLAYER_GUI_MOUNT]);

	CMenuForwarder * mf_umount = new CMenuForwarder(LOCALE_NFS_UMOUNT, true, NULL, new CNFSUmountGui(), NULL, CRCInput::RC_blue);
	mf_umount->setHint("", LOCALE_MENU_HINT_NET_NFS_UMOUNT);
	personalize->addItem(multimedia_menu, mf_umount, &g_settings.personalize[SNeutrinoSettings::P_MPLAYER_GUI_MOUNT]);
#endif

#if ENABLE_UPNP
	//upnp browser
	static CUpnpBrowserGui *upnpbrowsergui = NULL;
	if (!upnpbrowsergui)
		upnpbrowsergui = new CUpnpBrowserGui();

	CMenuForwarder *fw_upnp = new CMenuForwarder(LOCALE_UPNPBROWSER_HEAD, enabled, NULL, upnpbrowsergui, NULL);
	fw_upnp->setHint(NEUTRINO_ICON_HINT_A_PIC, LOCALE_MENU_HINT_UPNP);
	personalize->addItem(multimedia_menu, fw_upnp, &g_settings.personalize[SNeutrinoSettings::P_MEDIA_UPNP]);
#endif

	int res = menu_return::RETURN_NONE;
	
	if (show)
	{
 		//adding personalized items
		personalize->addPersonalizedItems();
		
		//add PLUGIN_INTEGRATION_MULTIMEDIA plugins
		unsigned int nextShortcut = (unsigned int)multimedia_menu->getNextShortcut();
		multimedia_menu->integratePlugins(PLUGIN_INTEGRATION_MULTIMEDIA, nextShortcut, enabled);

		res = multimedia_menu->exec(NULL, "");

		delete multimedia_menu;
		delete personalize;
	}
	return res;
}
