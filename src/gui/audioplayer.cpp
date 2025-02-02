/*
	Neutrino-GUI - DBoxII-Project - AudioPlayer by Dirch, Zwen

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Copyright (C) 2002-2008 the tuxbox project contributors
	Copyright (C) 2008 Novell, Inc. Author: Stefan Seyfried
	Copyright (C) 2011-2013,2015,2017 Stefan Seyfried
	Copyright (C) 2017 Sven Hoefer

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gui/audioplayer.h"

#include <global.h>
#include <neutrino.h>

#include <driver/display.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/audioplay.h>
#include <driver/audiometadata.h>

#include <daemonc/remotecontrol.h>

#include <gui/eventlist.h>
#include <gui/color.h>
#include <gui/infoviewer.h>

#ifdef ENABLE_GUI_MOUNT
#include <gui/nfs.h>
#endif

#include <gui/components/cc.h>
#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/msgbox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/keyboard_input.h>
#include <gui/screensaver.h>
#include "gui/pictureviewer.h"
extern CPictureViewer * g_PicViewer;
#include <gui/infoclock.h>
#include <system/settings.h>
#include <system/helpers.h>
#include <system/httptool.h>
#include <driver/screen_max.h>

#include <algorithm>
#include <sys/time.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <dirent.h>

#include <curl/curl.h>
#include <curl/easy.h>
#if LIBCURL_VERSION_NUM < 0x071507
#include <curl/types.h>
#endif

#include <zapit/zapit.h>
#include <libdvbapi/video.h>
extern cVideo * videoDecoder;

#define AUDIOPLAYERGUI_SMSKEY_TIMEOUT 1000
#define SHOW_FILE_LOAD_LIMIT 50

#define AUDIOPLAYER_TIME_DEBUG

// check if files to be added are already in the playlist
#define AUDIOPLAYER_CHECK_FOR_DUPLICATES
#define AUDIOPLAYER_START_SCRIPT "audioplayer.start"
#define AUDIOPLAYER_END_SCRIPT "audioplayer.end"
#define DEFAULT_RADIOSTATIONS_XMLFILE CONFIGDIR "/radio-stations.xml"
#define DEFAULT_RADIOFAVORITES_XMLFILE CONFIGDIR "/radio-favorites.xml"

const char RADIO_STATION_XML_FILE[] = {DEFAULT_RADIOSTATIONS_XMLFILE};
const char RADIO_FAVORITES_XML_FILE[] = {DEFAULT_RADIOFAVORITES_XMLFILE};

CAudiofileExt::CAudiofileExt() : CAudiofile(), firstChar('\0')
{
}

CAudiofileExt::CAudiofileExt(std::string name, CFile::FileType type) : CAudiofile(name, type), firstChar('\0')
{
}

CAudiofileExt::CAudiofileExt(const CAudiofileExt& src) : CAudiofile(src), firstChar(src.firstChar)
{
}

void CAudiofileExt::operator=(const CAudiofileExt& src)
{
	if (&src == this)
		return;
	CAudiofile::operator=(src);
	firstChar = src.firstChar;
}

CAudioPlayerGui::CAudioPlayerGui(bool inetmode)
{
	m_frameBuffer = CFrameBuffer::getInstance();
	m_visible = false;
	m_inetmode = inetmode;
	m_detailsline = NULL;
	m_infobox = NULL;
	m_titlebox = NULL;

	Init();
}

void CAudioPlayerGui::Init(void)
{
	m_selected = 0;
	m_metainfo.clear();

	pictureviewer = false;

	m_select_title_by_name = g_settings.audioplayer_select_title_by_name==1;

	if (!g_settings.network_nfs_audioplayerdir.empty())
		m_Path = g_settings.network_nfs_audioplayerdir.c_str();
	else
		m_Path = "/";

	audiofilefilter.Clear();
	if (m_inetmode)
	{
		audiofilefilter.addFilter("url");
		audiofilefilter.addFilter("xml");
		audiofilefilter.addFilter("m3u");
		audiofilefilter.addFilter("pls");
	}
	else
	{
		audiofilefilter.addFilter("cdr");
		audiofilefilter.addFilter("mp3");
		audiofilefilter.addFilter("m2a");
		audiofilefilter.addFilter("mpa");
		audiofilefilter.addFilter("mp2");
		audiofilefilter.addFilter("m3u");
		audiofilefilter.addFilter("ogg");
		audiofilefilter.addFilter("wav");
		audiofilefilter.addFilter("flac");
#ifdef ENABLE_FFMPEGDEC
		audiofilefilter.addFilter("flv");
		audiofilefilter.addFilter("aac");
		audiofilefilter.addFilter("dts");
		audiofilefilter.addFilter("m4a");
#endif
	}
	m_SMSKeyInput.setTimeout(AUDIOPLAYERGUI_SMSKEY_TIMEOUT);
}

CAudioPlayerGui::~CAudioPlayerGui()
{
	m_playlist.clear();
	m_radiolist.clear();
	m_filelist.clear();
	m_title2Pos.clear();
	delete m_detailsline;
	delete m_infobox;
	delete m_titlebox;
}

/*const*/ struct button_label AudioPlayerButtons[][4] =
{
	{
		{ NEUTRINO_ICON_BUTTON_STOP	, LOCALE_AUDIOPLAYER_STOP			},
		{ NEUTRINO_ICON_BUTTON_BACKWARD	, LOCALE_AUDIOPLAYER_REWIND			},
		{ NEUTRINO_ICON_BUTTON_PAUSE	, LOCALE_AUDIOPLAYER_PAUSE			},
		{ NEUTRINO_ICON_BUTTON_FORWARD	, LOCALE_AUDIOPLAYER_FASTFORWARD		}
	},
	{
		{ NEUTRINO_ICON_BUTTON_RED	, LOCALE_AUDIOPLAYER_DELETE			},
		{ NEUTRINO_ICON_BUTTON_GREEN	, LOCALE_AUDIOPLAYER_ADD			},
		{ NEUTRINO_ICON_BUTTON_YELLOW	, LOCALE_AUDIOPLAYER_DELETEALL			},
		{ NEUTRINO_ICON_BUTTON_BLUE	, LOCALE_AUDIOPLAYER_SHUFFLE			}
	},
	{
		{ NEUTRINO_ICON_BUTTON_GREEN	, LOCALE_AUDIOPLAYER_JUMP_BACKWARDS		},
		{ NEUTRINO_ICON_BUTTON_BLUE	, LOCALE_AUDIOPLAYER_JUMP_FORWARDS		}
	},
	{
		{ NEUTRINO_ICON_BUTTON_GREEN	, LOCALE_AUDIOPLAYER_JUMP_BACKWARDS		},
		{ NEUTRINO_ICON_BUTTON_BLUE	, LOCALE_AUDIOPLAYER_JUMP_FORWARDS		}
	},
	{
		{ NEUTRINO_ICON_BUTTON_GREEN	, LOCALE_AUDIOPLAYER_SAVE_PLAYLIST		},
		{ NEUTRINO_ICON_BUTTON_YELLOW	, LOCALE_AUDIOPLAYER_BUTTON_SELECT_TITLE_BY_ID	}
	},
	{
		{ NEUTRINO_ICON_BUTTON_GREEN	, LOCALE_AUDIOPLAYER_SAVE_PLAYLIST		},
		{ NEUTRINO_ICON_BUTTON_YELLOW	, LOCALE_AUDIOPLAYER_BUTTON_SELECT_TITLE_BY_NAME}
	},
	{
		{ NEUTRINO_ICON_BUTTON_STOP	, LOCALE_AUDIOPLAYER_STOP			},
		{ NEUTRINO_ICON_BUTTON_PAUSE	, LOCALE_AUDIOPLAYER_PAUSE			},
		{ NEUTRINO_ICON_BUTTON_RECORD_ACTIVE, LOCALE_AUDIOPLAYER_STREAMRIPPER_START	}
	},
	{
		{ NEUTRINO_ICON_BUTTON_GREEN	, LOCALE_AUDIOPLAYER_ADD			},
		{ NEUTRINO_ICON_BUTTON_BLUE	, LOCALE_INETRADIO_NAME				}
	},
	{
		{ NEUTRINO_ICON_BUTTON_RED	, LOCALE_AUDIOPLAYER_DELETE			},
		{ NEUTRINO_ICON_BUTTON_GREEN	, LOCALE_AUDIOPLAYER_ADD			},
		{ NEUTRINO_ICON_BUTTON_YELLOW	, LOCALE_AUDIOPLAYER_DELETEALL			},
		{ NEUTRINO_ICON_BUTTON_BLUE	, LOCALE_INETRADIO_NAME				}
	}
};

int CAudioPlayerGui::exec(CMenuTarget* parent, const std::string &actionKey)
{

	if (actionKey == "init")
		Init();

	CNeutrinoApp::getInstance()->StopSubtitles();

	CAudioPlayer::getInstance()->init();
	m_state = CAudioPlayerGui::STOP;

	m_show_playlist = g_settings.audioplayer_show_playlist==1;

	if (m_select_title_by_name != (g_settings.audioplayer_select_title_by_name==1))
	{
		if ((g_settings.audioplayer_select_title_by_name == 1) && m_playlistHasChanged)
		{
			buildSearchTree();
		}
		m_select_title_by_name = g_settings.audioplayer_select_title_by_name;
	}

	//auto-load favorites
	if ((m_inetmode) && (m_playlist.empty()))
	{
		if (access(RADIO_FAVORITES_XML_FILE, F_OK) == 0)
			scanXmlFile(RADIO_FAVORITES_XML_FILE);
	}

	if (m_playlist.empty())
		m_current = -1;
	else
		m_current = 0;

	m_selected = 0;

	m_width = m_frameBuffer->getWindowWidth();
	m_height = m_frameBuffer->getWindowHeight();

	m_header_height = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	m_item_height = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();

	m_meta_height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
	m_title_height = 3*OFFSET_INNER_SMALL + 2*m_item_height + m_meta_height;
	m_cover_width = 0;
	m_info_height = 2*OFFSET_INNER_SMALL + 2*g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight();
	m_button_height = ::paintButtons(AudioPlayerButtons[0], 4, 0, 0, 0, 0, 0, false, NULL, NULL);

	m_listmaxshow = (m_height - m_title_height - OFFSET_SHADOW - OFFSET_INTER - m_header_height - 2*m_button_height - OFFSET_SHADOW - OFFSET_INTER - m_info_height - OFFSET_SHADOW) / (m_item_height);
	m_height = m_title_height + OFFSET_SHADOW + OFFSET_INTER + m_header_height + m_listmaxshow*m_item_height + 2*m_button_height + OFFSET_SHADOW + OFFSET_INTER + m_info_height + OFFSET_SHADOW; // recalc height

	m_x = getScreenStartX(m_width);
	if (m_x < DETAILSLINE_WIDTH)
		m_x = DETAILSLINE_WIDTH;
	m_y = getScreenStartY(m_height);

	m_cover.clear();
	m_stationlogo = false;

	m_streamripper_available = !find_executable("streamripper").empty() && !find_executable("streamripper.sh").empty();
	m_streamripper_active = false;

	if (parent)
		parent->hide();

	bool usedBackground = m_frameBuffer->getuseBackground();
	if (usedBackground)
		m_frameBuffer->saveBackgroundImage();

	// set zapit in lock mode
	CNeutrinoApp::getInstance()->stopPlayBack(true);

	m_frameBuffer->showFrame("mp3.jpg");

	// tell neutrino we're in audio mode
	m_LastMode = CNeutrinoApp::getInstance()->getMode();
	CNeutrinoApp::getInstance()->handleMsg(NeutrinoMessages::CHANGEMODE , NeutrinoModes::mode_audio);

	// wake up hdd
	printf("[audioplayer.cpp] wakeup_hdd(%s)\n", g_settings.network_nfs_audioplayerdir.c_str());
	wakeup_hdd(g_settings.network_nfs_audioplayerdir.c_str()/*,true*/);

	exec_controlscript(AUDIOPLAYER_START_SCRIPT);

	int res = show();

	// Restore previous background
	if (usedBackground)
		m_frameBuffer->restoreBackgroundImage();
	m_frameBuffer->useBackground(usedBackground);
	m_frameBuffer->paintBackground();

	exec_controlscript(AUDIOPLAYER_END_SCRIPT);

	//g_Zapit->unlockPlayBack();
	CZapit::getInstance()->EnablePlayback(true);

	m_frameBuffer->stopFrame();
	CNeutrinoApp::getInstance()->handleMsg(NeutrinoMessages::CHANGEMODE , m_LastMode);
	g_RCInput->postMsg(NeutrinoMessages::SHOW_INFOBAR, 0);

	CNeutrinoApp::getInstance()->StartSubtitles();

#ifdef ENABLE_GRAPHLCD
	cGLCD::MirrorOSD(g_settings.glcd_mirror_osd);
	cGLCD::unlockChannel();
	cGLCD::unlockDuration();
	cGLCD::unlockStart();
	cGLCD::unlockEnd();
	cGLCD::ShowLcdIcon(false);
#endif
	return res;
}

int CAudioPlayerGui::show()
{
	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	int ret = menu_return::RETURN_REPAINT;

	// clear whole screen
	m_frameBuffer->paintBackground();
	CInfoClock::getInstance()->block();
	CScreenSaver::getInstance()->OnAfterStop.connect(sigc::mem_fun(CInfoClock::getInstance(), &CInfoClock::block));
	CScreenSaver::getInstance()->resetIdleTime();
	CVFD::getInstance()->setMode(CVFD::MODE_AUDIO);
	paintLCD();

	bool loop = true;
	bool update = true;
	bool clear_before_update = false;
	m_key_level = 0;

	// auto-play first entry from favorites
	if (g_settings.inetradio_autostart)
	{
		if ((m_inetmode) && (!m_playlist.empty()))
		{
			m_current = 0;
			m_selected = 0;
			play(m_selected);
		}
	}

	while (loop)
	{
		if (msg <= CRCInput::RC_MaxRC)
			CScreenSaver::getInstance()->resetIdleTime();

		updateMetaData();
		updateTimes();

		// stop if mode was changed in another thread
		if (CNeutrinoApp::getInstance()->getMode() != NeutrinoModes::mode_audio)
			loop = false;

		if (
			(m_state != CAudioPlayerGui::STOP) &&
			(CAudioPlayer::getInstance()->getState() == CBaseDec::STOP) &&
			(!m_playlist.empty())
		)
		{
			if (m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
				playNext();
		}

		if (m_streamripper_active && !getpidof("streamripper"))
		{
			printf("streamripper should but doesn't work.\n");
			m_streamripper_active = false;
			update = true;
		}

		if (update)
		{
			if (clear_before_update)
			{
				hide();
				clear_before_update = false;
			}
			update = false;
			paint();
		}
		g_RCInput->getMsg(&msg, &data, 10); // 1 sec timeout to update play/stop state display

		if (msg == CRCInput::RC_playpause)
		{
			// manipulate msg
			if (m_state == CAudioPlayerGui::PAUSE)
				msg = CRCInput::RC_play;
			else
				msg = CRCInput::RC_pause;
		}

		if (msg == CRCInput::RC_timeout || msg == NeutrinoMessages::EVT_TIMER)
		{
			if (CScreenSaver::getInstance()->canStart() && !CScreenSaver::getInstance()->isActive())
			{
				CScreenSaver::getInstance()->Start();
			}
		}
		else if (!CScreenSaver::getInstance()->ignoredMsg(msg))
		{
			if (CScreenSaver::getInstance()->isActive())
			{
				printf("[audioplayer.cpp] CScreenSaver stop; msg: %lX\n", msg);
				CScreenSaver::getInstance()->Stop();

				m_frameBuffer->stopFrame();
				m_frameBuffer->showFrame("mp3.jpg");
				paint();

				if (msg <= CRCInput::RC_MaxRC)
				{
					// ignore first keypress - just quit the screensaver
					g_RCInput->clearRCMsg();
					continue;
				}
			}
		}

		if (msg == CRCInput::RC_home)
		{
			if (m_state == CAudioPlayerGui::STOP)
				loop=false;
		}
		else if (msg == CRCInput::RC_stop)
		{
			if (m_state != CAudioPlayerGui::STOP)
				stop();
		}
		//add key_favorites for internetradio
		else if ((msg == (neutrino_msg_t) g_settings.key_favorites) && (m_inetmode))
		{
			if (m_key_level == 0)
			{
				// clear playlist and load RADIO_FAVORITES_XML_FILE
				if (access(RADIO_FAVORITES_XML_FILE, F_OK) == 0)
				{
					if (!m_playlist.empty())
						clearPlaylist();
					scanXmlFile(RADIO_FAVORITES_XML_FILE);
					clear_before_update = true;
					update = true;
				}
			}
		}
		else if (msg == CRCInput::RC_left || msg == CRCInput::RC_previoussong)
		{
			if (m_key_level == 1)
			{
				playPrev();
			}
			else
			{
				if (!m_show_playlist)
				{
					m_current--;
					if (m_current < 0)
						m_current = m_playlist.size()-1;
					update = true;
				}
				else if (!m_playlist.empty())
				{
					if (m_selected < m_listmaxshow)
						m_selected = m_playlist.size()-1;
					else
						m_selected -= m_listmaxshow;
					m_liststart = (m_selected / m_listmaxshow) * m_listmaxshow;
					update = true;
				}
			}

		}
		else if (msg == CRCInput::RC_right || msg == CRCInput::RC_nextsong)
		{
			if (m_key_level == 1)
			{
				playNext();
			}
			else
			{
				if (!m_show_playlist)
				{
					m_current++;
					if (m_current >= (int)m_playlist.size())
						m_current = 0;
					update = true;
				}
				else if (!m_playlist.empty())
				{
					m_selected += m_listmaxshow;
					if (m_selected >= m_playlist.size())
					{
						if (((m_playlist.size() / m_listmaxshow) + 1) * m_listmaxshow == m_playlist.size() + m_listmaxshow)
							m_selected = 0;
						else
							m_selected = m_selected < (((m_playlist.size() / m_listmaxshow) + 1) * m_listmaxshow) ? (m_playlist.size()-1) : 0;
					}
					m_liststart = (m_selected / m_listmaxshow) * m_listmaxshow;
					update = true;
				}
			}
		}
		else if ((msg &~ CRCInput::RC_Repeat) == CRCInput::RC_up || (msg &~ CRCInput::RC_Repeat) == CRCInput::RC_page_up)
		{
			if (m_show_playlist && !m_playlist.empty())
			{
				int prevselected = m_selected;
				int step = msg == CRCInput::RC_page_up ? m_listmaxshow : 1;

				m_selected -= step;
				if ((prevselected-step) < 0)
					m_selected = m_playlist.size()-1;

				unsigned int oldliststart = m_liststart;
				m_liststart = (m_selected/m_listmaxshow)*m_listmaxshow;
				if (oldliststart != m_liststart)
				{
					update = true;
				}
				else
				{
					paintItem(prevselected - m_liststart);
					paintItem(m_selected - m_liststart);
				}
			}
		}
		else if ((msg &~ CRCInput::RC_Repeat) == CRCInput::RC_down || (msg &~ CRCInput::RC_Repeat) == CRCInput::RC_page_down)
		{
			if (m_show_playlist && !m_playlist.empty())
			{
				int prevselected = m_selected;
				unsigned int step = msg == CRCInput::RC_page_down ? m_listmaxshow : 1;
				m_selected += step;

				if (m_selected >= m_playlist.size())
				{
					if (((m_playlist.size() / m_listmaxshow) + 1) * m_listmaxshow == m_playlist.size() + m_listmaxshow) // last page has full entries
						m_selected = 0;
					else
						m_selected = ((step == m_listmaxshow) && (m_selected < (((m_playlist.size() / m_listmaxshow)+1) * m_listmaxshow))) ? (m_playlist.size() - 1) : 0;
				}
				//m_selected = (m_selected + 1) % m_playlist.size();

				unsigned int oldliststart = m_liststart;
				m_liststart = (m_selected/m_listmaxshow)*m_listmaxshow;
				if (oldliststart != m_liststart)
				{
					update = true;
				}
				else
				{
					paintItem(prevselected - m_liststart);
					paintItem(m_selected - m_liststart);
				}
			}
		}
		else if (msg == CRCInput::RC_ok || msg == CRCInput::RC_play)
		{
			if (!m_playlist.empty())
			{
				if (!m_show_playlist)
					play(m_current);
				else
					play(m_selected);
			}
		}
		else if (msg == CRCInput::RC_forward && m_key_level == 1 && m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
			ff();
		else if (msg == CRCInput::RC_rewind && m_key_level == 1 && m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
			rev();
		else if (msg == CRCInput::RC_stop && m_key_level == 1 && m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
			stop();
		else if (msg == CRCInput::RC_pause && m_key_level == 1 && m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
			pause();
		else if (msg == CRCInput::RC_next && m_key_level == 1 && m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
			playNext();
		else if (msg == CRCInput::RC_prev && m_key_level == 1 && m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
			playPrev();
		else if (msg == CRCInput::RC_red)
		{
			if (m_key_level == 0)
			{
				if (!m_playlist.empty())
				{
					removeFromPlaylist(m_selected);
					if ((int)m_selected == m_current)
						m_current--;

					if (m_selected >= m_playlist.size())
						m_selected = m_playlist.empty() ? m_playlist.size() : m_playlist.size() - 1;
					update = true;
				}
			}
			else if (m_key_level == 1)
			{
				stop();
			}
			else
			{
				// key_level==2
			}

		}
		else if (msg == CRCInput::RC_stop)
		{
			if (m_key_level == 1)
				stop();
		}
		else if ((msg == CRCInput::RC_rewind) || (msg == CRCInput::RC_green))
		{
			if (m_key_level == 0)
			{
				openFilebrowser();
				update=true;
			}
			else if (m_key_level == 1)
			{
				if (m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
					rev();
			}
			else
			{ // key_level == 2

				if (m_state == CAudioPlayerGui::STOP)
				{
					if (!m_playlist.empty())
					{
						savePlaylist();
						CVFD::getInstance()->setMode(CVFD::MODE_AUDIO);
						paintLCD();
						update = true;
					}
				}
				else
				{
					// keylevel 2 can only be reached if the currently played file
					// is no stream, so we do not have to test for this case
					int seconds=0;
					CIntInput secondsInput(	LOCALE_AUDIOPLAYER_JUMP_DIALOG_TITLE,
								&seconds,
								5,
								LOCALE_AUDIOPLAYER_JUMP_DIALOG_HINT1,
								LOCALE_AUDIOPLAYER_JUMP_DIALOG_HINT2);
					int res = secondsInput.exec(NULL,"");
					if (seconds != 0 && res!= menu_return::RETURN_EXIT_ALL)
						rev(seconds);
					update=true;
				}
			}
		}
		else if ((msg == CRCInput::RC_pause) || (msg == CRCInput::RC_yellow))
		{
			if (m_key_level == 0)
			{
				if (!m_playlist.empty())
				{
					//stop();
					clearPlaylist();
					clear_before_update = true;
					update = true;
				}
			}
			else if (m_key_level == 1)
			{
				pause();
			}
			else
			{ // key_level==2
				m_select_title_by_name =! m_select_title_by_name;
				if (m_select_title_by_name && m_playlistHasChanged)
					buildSearchTree();
				paint();
			}
		}
		else if ((msg == CRCInput::RC_forward) || (msg == CRCInput::RC_blue))
		{
			if (m_key_level == 0)
			{
				if (m_inetmode)
				{
					static int old_select = 0;
					char cnt[5];
					CMenuWidget InputSelector(LOCALE_AUDIOPLAYER_LOAD_RADIO_STATIONS, NEUTRINO_ICON_AUDIO);
					int count = 0;
					int select = -1;
					CMenuSelectorTarget *InetRadioInputChanger = new CMenuSelectorTarget(&select);
					// -- setup menue for inetradio input
					sprintf(cnt, "%d", count);
					InputSelector.addItem(new CMenuForwarder(
								LOCALE_AUDIOPLAYER_ADD_FAV, true, NULL, InetRadioInputChanger,
								cnt, CRCInput::convertDigitToKey(count + 1)), old_select == count);
					sprintf(cnt, "%d", ++count);
					InputSelector.addItem(new CMenuForwarder(
								LOCALE_AUDIOPLAYER_ADD_LOC, true, NULL, InetRadioInputChanger,
								cnt, CRCInput::convertDigitToKey(count + 1)), old_select == count);
					sprintf(cnt, "%d", ++count);
					InputSelector.addItem(new CMenuForwarder(
								LOCALE_AUDIOPLAYER_ADD_IC, true, NULL, InetRadioInputChanger,
								cnt, CRCInput::convertDigitToKey(count + 1)), old_select == count);
					sprintf(cnt, "%d", ++count);
					InputSelector.addItem(new CMenuForwarder(
								LOCALE_AUDIOPLAYER_ADD_SC, g_settings.shoutcast_enabled, NULL, InetRadioInputChanger,
								cnt, CRCInput::convertDigitToKey(count + 1)), old_select == count);

					//InputSelector.addItem(GenericMenuSeparator);
					hide();
					InputSelector.exec(NULL, "");

					delete InetRadioInputChanger;

					if (!m_playlist.empty())
						clearPlaylist();

					if (select >= 0)
						old_select = select;
					switch (select)
					{
						case 0:
							scanXmlFile(RADIO_FAVORITES_XML_FILE);
							CVFD::getInstance()->setMode(CVFD::MODE_AUDIO);
							paintLCD();
							break;
						case 1:
							scanXmlFile(RADIO_STATION_XML_FILE);
							CVFD::getInstance()->setMode(CVFD::MODE_AUDIO);
							paintLCD();
							break;
						case 2:
							readDir_ic();
							CVFD::getInstance()->setMode(CVFD::MODE_AUDIO);
							paintLCD();
							break;
						case 3:
							openSCbrowser();
							break;
						default:
							break;
					}
					m_current = 0;
					m_selected = 0;
					update=true;
				}
				else if (shufflePlaylist())
				{
					update = true;
				}
			}
			else if (m_key_level == 1)
			{
				if (m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
					ff();
			}
			else // key_level == 2
			{
				if (m_state != CAudioPlayerGui::STOP)
				{
					// keylevel 2 can only be reached if the currently played file
					// is no stream, so we do not have to test for this case
					int seconds=0;
					CIntInput secondsInput(	LOCALE_AUDIOPLAYER_JUMP_DIALOG_TITLE,
								&seconds,
								5,
								LOCALE_AUDIOPLAYER_JUMP_DIALOG_HINT1,
								LOCALE_AUDIOPLAYER_JUMP_DIALOG_HINT2);
					int res = secondsInput.exec(NULL,"");
					if (seconds != 0 && res!= menu_return::RETURN_EXIT_ALL)
						ff(seconds);
					update = true;
				}
			}
		}
#if 0
		// FIXME; this seems totally broken
		else if (msg == CRCInput::RC_info && !m_playlist.empty())
		{
			pictureviewer = true;
			m_frameBuffer->Clear();
			m_frameBuffer->stopFrame();
			CPictureViewerGui * picture = new CPictureViewerGui();
			picture->m_audioPlayer = this;
			picture->exec(this, "audio");
			delete picture;
			pictureviewer = false;
			CScreenSaver::getInstance()->resetIdleTime();
			videoDecoder->setBlank(true);
			m_frameBuffer->showFrame("mp3.jpg");
			CVFD::getInstance()->setMode(CVFD::MODE_AUDIO);
			paintLCD();
			update = true;
		}
#endif
		else if (msg == CRCInput::RC_help)
		{
			if (m_key_level == 2)
				m_key_level = 0;
			else
				m_key_level++;

			if (m_state != CAudioPlayerGui::STOP)
			{
				// jumping in streams not supported
				if (m_key_level == 2 && m_curr_audiofile.FileType == CFile::STREAM_AUDIO)
				{
					m_key_level = 0;
				}
			}
			// there are only two keylevels in the "STOP"-case
			else if (m_key_level == 1)
			{
				m_key_level = 2;
			}
			paintFoot();
		}
		else if (msg == CRCInput::RC_0)
		{
			if (m_current >= 0)
			{
				m_selected = m_current;
				update = true;
			}
		}
		else if (msg == (neutrino_msg_t) g_settings.key_record)
		{
			if (m_key_level == 1)
			{
				if (m_curr_audiofile.FileType == CFile::STREAM_AUDIO && m_streamripper_available)
				{
					if (m_streamripper_active)
					{
						ShowHint(LOCALE_MESSAGEBOX_INFO, LOCALE_AUDIOPLAYER_STREAMRIPPER_STOP, HINTBOX_MIN_WIDTH, 2);
						my_system(2, "streamripper.sh", "stop");
						m_streamripper_active = false;
					}
					else
					{
						ShowHint(LOCALE_MESSAGEBOX_INFO, LOCALE_AUDIOPLAYER_STREAMRIPPER_START, HINTBOX_MIN_WIDTH, 2);
						printf("streamripper.sh start \"%s\"\n", m_playlist[m_current].MetaData.url.c_str());
						puts("[audioplayer.cpp] executing streamripper");
						if (my_system(3, "streamripper.sh", "start", m_playlist[m_current].MetaData.url.c_str()) != 0)
							perror("[audioplayer.cpp]: streamripper.sh failed\n");
						else
							m_streamripper_active = true;
					}
					update = true;
				}
			}
		}
		else if (CRCInput::isNumeric(msg) && !(m_playlist.empty()))
		{ //numeric zap or SMS zap
			if (m_select_title_by_name)
			{
				//printf("select by name\n");
				unsigned char smsKey = 0;
				int w = 0;
				do
				{
					//FIXME - remove fixed values

					smsKey = m_SMSKeyInput.handleMsg(msg);
					//printf(" new key: %c", smsKey);
					/* show a hint box with current char (too slow at the moment?)*/
					char selectedKey[2];
					sprintf(selectedKey,"%c",smsKey);
					int x1=(g_settings.screen_EndX- g_settings.screen_StartX)/2 + g_settings.screen_StartX-50;
					int y1=(g_settings.screen_EndY- g_settings.screen_StartY)/2 + g_settings.screen_StartY;
					int h = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getHeight();
					w = std::max(w, g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getRenderWidth(selectedKey));
					m_frameBuffer->paintBoxRel(x1 - 7, y1 - h - 5, w + 14, h + 10, COL_FRAME_PLUS_0, RADIUS_SMALL);
					m_frameBuffer->paintBoxRel(x1 - 6, y1 - h - 4, w + 12, h +  8, COL_MENUCONTENTSELECTED_PLUS_0, RADIUS_SMALL);
					g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->RenderString(x1,y1,w+1,selectedKey,COL_MENUCONTENTSELECTED_TEXT);

					g_RCInput->getMsg_ms(&msg, &data, AUDIOPLAYERGUI_SMSKEY_TIMEOUT - 200);

				} while (CRCInput::isNumeric(msg) && !(m_playlist.empty()));

				if (msg == CRCInput::RC_timeout || msg == CRCInput::RC_nokey)
				{
					//printf("selected key: %c\n",smsKey);
					selectTitle(smsKey);
				}
				update = true;
				m_SMSKeyInput.resetOldKey();
			}
			else
			{
				//printf("numeric zap\n");
				int val = 0;
				if (getNumericInput(msg,val))
					m_selected = std::min((int)m_playlist.size(), val) - 1;
				update = true;
			}

		}

#ifdef ENABLE_GUI_MOUNT
		else if ((msg == CRCInput::RC_setup) && !m_inetmode)
		{
			CNFSSmallMenu nfsMenu;
			nfsMenu.exec(this, "");
			CVFD::getInstance()->setMode(CVFD::MODE_AUDIO);
			paintLCD();
			update = true;
		}
#endif
		else if (msg == NeutrinoMessages::CHANGEMODE)
		{
			if ((data & NeutrinoModes::mode_mask) != NeutrinoModes::mode_audio)
			{
				loop = false;
				m_LastMode=data;
			}
		}
		else if (
				msg == NeutrinoMessages::RECORD_START ||
				msg == NeutrinoMessages::ZAPTO ||
				msg == NeutrinoMessages::STANDBY_ON ||
				msg == NeutrinoMessages::LEAVE_ALL ||
				msg == NeutrinoMessages::SHUTDOWN ||
				(msg == NeutrinoMessages::SLEEPTIMER && !data)
		)
		{
			if (msg != NeutrinoMessages::RECORD_START)
				ret = menu_return::RETURN_EXIT_ALL;
			// Exit for Record/Zapto Timers
			loop = false;

			g_RCInput->postMsg(msg, data);
		}
		else if (msg == NeutrinoMessages::EVT_TIMER)
		{
			CNeutrinoApp::getInstance()->handleMsg(msg, data);
		}
		else
		{
			if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
			{
				ret = menu_return::RETURN_EXIT_ALL;
				loop = false;
			}
			//paintLCD();
		}
		m_frameBuffer->blit();
	}
	hide();

	if (m_state != CAudioPlayerGui::STOP)
		stop();
	CInfoClock::getInstance()->enableInfoClock(CInfoClock::getInstance()->isRun());
	CScreenSaver::getInstance()->OnAfterStop.clear();
	return ret;
}

bool CAudioPlayerGui::playNext(bool allow_rotate)
{
	bool result = false;

	if (!(m_playlist.empty()))
	{
		int next = getNext();
		if (next >= 0)
			play(next);
		else if (allow_rotate)
			play(0);
		else
			stop();
		result = true;
	}

	return(result);
}

void CAudioPlayerGui::wantNextPlay()
{
	if (
		(m_state != CAudioPlayerGui::STOP) &&
		(CAudioPlayer::getInstance()->getState() == CBaseDec::STOP) &&
		(!m_playlist.empty())
	)
	{
		if (m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
			playNext();
	}
}

bool CAudioPlayerGui::playPrev(bool allow_rotate)
{
	bool result = false;

	if (!(m_playlist.empty()))
	{
		if (m_current == -1)
			stop();
		else if (m_current-1 > 0)
			play(m_current-1);
		else if (allow_rotate)
			play(m_playlist.size()-1);
		else
			play(0);
		result = true;
	}

	return(result);
}

bool CAudioPlayerGui::clearPlaylist(void)
{
	bool result = false;

	CAudioPlayList::const_iterator it;
#if 0
	for (it = m_playlist.begin(); it!=m_playlist.end(); ++it) {
		unlink(it->MetaData.cover.c_str());
	}
#endif

	if (!(m_playlist.empty()))
	{
		m_playlist.clear();
		m_current = -1;
		m_selected = 0;
		m_title2Pos.clear();
		result = true;
	}
	return(result);
}

bool CAudioPlayerGui::shufflePlaylist(void)
{
	RandomNumber rnd;
	bool result = false;
	if (!(m_playlist.empty()))
	{
		if (m_current > 0)
		{
			std::swap(m_playlist[0], m_playlist[m_current]);
			m_current = 0;
		}

		std::random_shuffle((m_current != 0) ? m_playlist.begin() : m_playlist.begin() + 1, m_playlist.end(), rnd);
		if (m_select_title_by_name)
			buildSearchTree();

		m_playlistHasChanged = true;
		m_selected = 0;

		result = true;
	}
	return(result);
}

void CAudioPlayerGui::addUrl2Playlist(const char *url, const char *name, const char *logo, const time_t bitrate)
{
	CAudiofileExt mp3(url, CFile::STREAM_AUDIO);
	//tmp = tmp.substr(0,tmp.length()-4);	//remove .url
	//printf("[addUrl2Playlist], name = %s, url = %s\n", name, url);
	if (name != NULL)
	{
		mp3.MetaData.title = name;
	}
	else
	{
		std::string filename = mp3.Filename.substr(mp3.Filename.rfind('/') + 1);
		mp3.MetaData.title = filename;
	}

	if (logo != NULL)
		mp3.MetaData.logo = logo;
	else
		mp3.MetaData.logo.clear();

	if (bitrate)
		mp3.MetaData.total_time = bitrate;
	else
		mp3.MetaData.total_time = 0;

	mp3.MetaData.url = url;

	if (url[0] != '#')
		addToPlaylist(mp3);
}

void CAudioPlayerGui::processPlaylistUrl(const char *url, const char *name, const char *logo, const time_t tim)
{
	CURL *curl_handle;
	struct MemoryStruct chunk;
	const long int GET_PLAYLIST_TIMEOUT = 2;

	printf("CAudioPlayerGui::processPlaylistUrl (%s, %s)\n", url, name);
	chunk.memory=NULL; /* we expect realloc(NULL, size) to work */
	chunk.size = 0; /* no data at this point */

	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();

	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	/* send all data to this function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	/* some servers don't like requests that are made without a user-agent field, so we provide one */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* don't use signal for timeout */
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, (long)1);

	/* set timeout to 10 seconds */
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, GET_PLAYLIST_TIMEOUT);

	/* get it! */
	curl_easy_perform(curl_handle);

	/*
		Now, our chunk.memory points to a memory block that is chunk.size
		bytes big and contains the remote file.

		Do something nice with it!

		You should be aware of the fact that at this point we might have an
		allocated data block, and nothing has yet deallocated that data. So when
		you're done with it, you should free() it as a nice application.
	 */

	long res_code;
	if (curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE/*CURLINFO_RESPONSE_CODE*/, &res_code) == CURLE_OK)
	{
		if (200 == res_code)
		{
			//printf("\nchunk = %s\n", chunk.memory);
			std::istringstream iss;
			iss.str (std::string(chunk.memory, chunk.size));
			char line[512];
			char *ptr;
			while (iss.rdstate() == std::ifstream::goodbit)
			{
				iss.getline(line, 512);
				if (line[0] != '#')
				{
					//printf("chunk: line = %s\n", line);
					const char *schemes[] = {"http://", "https://", NULL };
					const char **scheme = schemes;
					while (*scheme) {
						ptr = strstr(line, *scheme);
						scheme++;
						if (ptr == NULL)
							continue;
						char *tmp;
						// strip \n and \r characters from url
						tmp = strchr(line, '\r');
						if (tmp != NULL)
							*tmp = '\0';
						tmp = strchr(line, '\n');
						if (tmp != NULL)
							*tmp = '\0';
						addUrl2Playlist(ptr, name, logo, tim);
						break;
					}
				}
			}
		}
	}

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	if (chunk.memory)
		free(chunk.memory);

	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();
}

void CAudioPlayerGui::readDir_ic(void)
{
	const std::string icecasturl = "http://dir.xiph.org/yp.xml";
	const long int GET_ICECAST_TIMEOUT = 90; // list is about 500kB!

	std::string answer="";
	std::cout << "[readDir_ic] IC URL: " << icecasturl << std::endl;
	CURL *curl_handle;
	CURLcode httpres;
	/* init the curl session */
	curl_handle = curl_easy_init();
	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, icecasturl.c_str());
	/* send all data to this function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, CurlWriteToString);
	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_FILE, (void *)&answer);
	/* Generate error if http error >= 400 occurs */
	curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1);
	/* set timeout to 30 seconds */
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, GET_ICECAST_TIMEOUT);

	/* error handling */
	char error[CURL_ERROR_SIZE];
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error);
	/* get it! */
	CHintBox *scanBox = new CHintBox(LOCALE_AUDIOPLAYER_ADD_IC, g_Locale->getText(LOCALE_AUDIOPLAYER_RECEIVING_LIST)); // UTF-8
	scanBox->paint();

	httpres = curl_easy_perform(curl_handle);

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	//std::cout << "Answer:" << std::endl << "----------------" << std::endl << answer << std::endl;

	if (!answer.empty() && httpres == 0)
	{
		xmlDocPtr answer_parser = parseXml(answer.c_str());
		scanBox->hide();
		scanXmlData(answer_parser, "listen_url", "server_name", "", "bitrate", true);
	}
	else
		scanBox->hide();

	delete scanBox;
}

void CAudioPlayerGui::scanXmlFile(std::string filename)
{
	xmlDocPtr answer_parser = parseXmlFile(filename.c_str());
	scanXmlData(answer_parser, "url", "name", "logo");
}

void CAudioPlayerGui::scanXmlData(xmlDocPtr answer_parser, const char *urltag, const char *nametag, const char *logotag, const char *bitratetag, bool usechild)
{
#define IC_typetag "server_type"

	if (answer_parser != NULL)
	{
		xmlNodePtr element = xmlDocGetRootElement(answer_parser);
		element = xmlChildrenNode(element);
		xmlNodePtr element_tmp = element;
		if (element == NULL)
		{
			printf("[openFilebrowser] No valid XML File.\n");
		}
		else
		{
			CProgressWindow progress;
			long maxProgress = 1;
			// count # of entries
			while (element)
			{
				maxProgress++;
				element = xmlNextNode(element);
			}
			element = element_tmp;
			long listPos = -1;
			progress.setTitle(LOCALE_AUDIOPLAYER_LOAD_RADIO_STATIONS);
			progress.exec(this, "");
			neutrino_msg_t msg;
			neutrino_msg_data_t data;
			g_RCInput->getMsg(&msg, &data, 0);
			while (element && msg != CRCInput::RC_home)
			{
				const char *ptr = NULL;
				const char *name = NULL;
				const char *url = NULL;
				const char *logo = NULL;

				time_t bitrate = 0;
				bool skip = true;
				listPos++;
				// show status
				int global = 100*listPos / maxProgress;
				progress.showStatus(global);
#ifdef LCD_UPDATE
				CVFD::getInstance()->showProgressBar(global, "read xmldata...");
				CVFD::getInstance()->setMode(CVFD::MODE_PROGRESSBAR);
#endif // LCD_UPDATE

				if (usechild)
				{
					const char *type = NULL;
					xmlNodePtr child = xmlChildrenNode(element);
					while (child)
					{
						if (strcmp(xmlGetName(child), urltag) == 0)
							url = xmlGetData(child);
						else if (strcmp(xmlGetName(child), nametag) == 0)
							name = xmlGetData(child);
						else if (strcmp(xmlGetName(child), logotag) == 0)
							logo = xmlGetData(child);
						else if (strcmp(xmlGetName(child), IC_typetag) == 0)
							type = xmlGetData(child);
						else if (bitratetag && strcmp(xmlGetName(child), bitratetag) == 0)
						{
							ptr = xmlGetData(child);
							if (ptr)
								bitrate = atoi(ptr);
						}
						child = xmlNextNode(child);
					}
					if (type)
					{
						if	(strcmp("audio/mpeg", type) == 0)	skip = false;
						else if	(strcmp("application/ogg", type) == 0)	skip = false;
						else if	(strcmp("mp3", type) == 0)		skip = false;
						else if	(strcmp("application/mp3", type) == 0)	skip = false;
					}
				}
				else
				{
					url = xmlGetAttribute(element, urltag);
					name = xmlGetAttribute(element, nametag);
					logo = xmlGetAttribute(element, logotag);
					if (bitratetag)
					{
						ptr = xmlGetAttribute(element, bitratetag);
						if (ptr)
							bitrate = atoi(ptr);
					}
					skip = false;
				}

				if ((url != NULL) && !skip)
				{
					progress.showStatusMessageUTF(url);
					//printf("Processing %s, %s\n", url, name);
					if (strstr(url, ".m3u") || strstr(url, ".pls"))
						processPlaylistUrl(url, name, logo);
					else
						addUrl2Playlist(url, name, logo, bitrate);
				}
				element = xmlNextNode(element);
				g_RCInput->getMsg(&msg, &data, 0);

				if( ( msg>= CRCInput::RC_WithData ) && ( msg< CRCInput::RC_WithData+ 0x10000000 ) )
					delete[] (unsigned char*) data;

			}
			progress.hide();
		}
		xmlFreeDoc(answer_parser);
	}
	else
		printf("[scanXmlData] answer_parser == NULL");
}

bool CAudioPlayerGui::openFilebrowser(void)
{
	bool result = false;
	CFileBrowser filebrowser((g_settings.filebrowser_denydirectoryleave) ? g_settings.network_nfs_audioplayerdir.c_str() : "");

	filebrowser.Multi_Select	= true;
	filebrowser.Dirs_Selectable	= true;
	filebrowser.Filter		= &audiofilefilter;

	hide();

	if (filebrowser.exec(m_Path.c_str()))
	{
#ifdef AUDIOPLAYER_TIME_DEBUG
		timeval start;
		gettimeofday(&start,NULL);
#endif
		CProgressWindow progress;
		long maxProgress = (filebrowser.getSelectedFiles().size() > 1) ? filebrowser.getSelectedFiles().size() - 1 : 1;
		long currentProgress = -1;
		if (maxProgress > SHOW_FILE_LOAD_LIMIT)
		{
			progress.setTitle(LOCALE_AUDIOPLAYER_READING_FILES);
			progress.exec(this,"");
		}

		m_Path = filebrowser.getCurrentDir();
		CFileList::const_iterator files = filebrowser.getSelectedFiles().begin();
		for (; files != filebrowser.getSelectedFiles().end(); ++files)
		{
			if (maxProgress > SHOW_FILE_LOAD_LIMIT)
			{
				currentProgress++;
				// show status
				int global = 100*currentProgress/maxProgress;
				progress.showStatus(global);
				progress.showStatusMessageUTF(files->Name);
#ifdef LCD_UPDATE
				CVFD::getInstance()->showProgressBar(global, "read metadata...");
				CVFD::getInstance()->setMode(CVFD::MODE_PROGRESSBAR);
#endif // LCD_UPDATE
			}
			if (
				(files->getType() == CFile::FILE_CDR)
				|| (files->getType() == CFile::FILE_OGG)
				|| (files->getType() == CFile::FILE_MP3)
				|| (files->getType() == CFile::FILE_WAV)
#ifdef ENABLE_FFMPEGDEC
					||  (files->getType() == CFile::FILE_AAC)
					||  (files->getType() == CFile::FILE_FLV)
#endif
					||  (files->getType() == CFile::FILE_FLAC)
			   )
			{
				CAudiofileExt audiofile(files->Name,
							files->getType());
				addToPlaylist(audiofile);
			}
			if (files->getType() == CFile::STREAM_AUDIO)
			{
				std::string filename = files->Name;
				FILE *fd = fopen(filename.c_str(), "r");
				if (fd)
				{
					char buf[512];
					unsigned int len = fread(buf, sizeof(char), 512, fd);
					fclose(fd);

					if (len && (strstr(buf, ".m3u") || strstr(buf, ".pls")))
					{
						printf("m3u/pls Playlist found: %s\n", buf);
						filename = buf;
						processPlaylistUrl(buf);
					}
					else
					{
						addUrl2Playlist(filename.c_str());
					}
				}
			}
			else if (files->getType() == CFile::FILE_PLAYLIST)
			{
				std::string sPath = files->Name.substr(0, files->Name.rfind('/'));
				std::ifstream infile;
				char cLine[256];
				char name[255] = { 0 };
				infile.open(files->Name.c_str(), std::ifstream::in);
				while (infile.good())
				{
					infile.getline(cLine, 255);
					int duration;
					sscanf(cLine, "#EXTINF:%d,%[^\n]\n", &duration, name);
					if (strlen(cLine) > 0 && cLine[0]!='#')
					{
						// remove CR
						if (cLine[strlen(cLine)-1]=='\r')
							cLine[strlen(cLine)-1]=0;
						char *url = strstr(cLine, "http://");
						if (url != NULL)
						{
							if (strstr(url, ".m3u") || strstr(url, ".pls"))
								processPlaylistUrl(url);
							else
								addUrl2Playlist(url, name, NULL, duration);
						}
						else if ((url = strstr(cLine, "icy://")) != NULL)
						{
							addUrl2Playlist(url);
						}
						else if ((url = strstr(cLine, "scast:://")) != NULL)
						{
							addUrl2Playlist(url);
						}
						else
						{
							std::string filename = sPath + '/' + cLine;

							std::string::size_type pos;
							while ((pos=filename.find('\\'))!=std::string::npos)
								filename[pos]='/';

							std::ifstream testfile;
							testfile.open(filename.c_str(), std::ifstream::in);
							if (testfile.good())
							{
#ifdef AUDIOPLAYER_CHECK_FOR_DUPLICATES
								// Check for duplicates and remove (new entry has higher prio)
								// this really needs some time :(
								for (unsigned long i=0; i<m_playlist.size(); i++)
								{
									if (m_playlist[i].Filename == filename)
										removeFromPlaylist(i);
								}
#endif
								if (strcasecmp(filename.substr(filename.length()-3,3).c_str(), "url")==0)
								{
									addUrl2Playlist(filename.c_str());
								}
								else
								{
									CFile playlistItem;
									playlistItem.Name = filename;
									CFile::FileType fileType = playlistItem.getType();
									if (fileType == CFile::FILE_CDR
											|| fileType == CFile::FILE_MP3
											|| fileType == CFile::FILE_OGG
											|| fileType == CFile::FILE_WAV
											|| fileType == CFile::FILE_FLAC
#ifdef ENABLE_FFMPEGDEC
											|| fileType == CFile::FILE_FLV
#endif
									   )
									{
										CAudiofileExt audioFile(filename,fileType);
										addToPlaylist(audioFile);
									}
									else
									{
										printf("Audioplayer: file type (%d) is *not* supported in playlists\n(%s)\n", fileType, filename.c_str());
									}
								}
							}
							testfile.close();
						}
					}
				}
				infile.close();
			}
			else if (files->getType() == CFile::FILE_XML)
			{
				if (!files->Name.empty())
					scanXmlFile(files->Name);
			}
		}
		if (m_select_title_by_name)
			buildSearchTree();

#ifdef AUDIOPLAYER_TIME_DEBUG
		timeval end;
		gettimeofday(&end,NULL);
		printf("adding %ld files took: ",maxProgress+1);
		printTimevalDiff(start,end);
#endif
		//store last dir
		if (g_settings.network_nfs_audioplayerdir.size() > m_Path.size() && g_settings.network_nfs_audioplayerdir != m_Path.c_str())
			g_settings.network_nfs_audioplayerdir = m_Path;

		result = true;
	}
	CScreenSaver::getInstance()->resetIdleTime();
	CVFD::getInstance()->setMode(CVFD::MODE_AUDIO);
	paintLCD();
	// if playlist is turned off -> start playing immediately
	if (!m_show_playlist && !m_playlist.empty())
		play(m_selected);

	return result;
}

bool CAudioPlayerGui::openSCbrowser(void)
{
	bool result = false;
	//shoutcast
	const char *sc_base_dir	= "http://api.shoutcast.com";

	CFileBrowser filebrowser(sc_base_dir, CFileBrowser::ModeSC);

	filebrowser.Multi_Select	= true;
	filebrowser.Dirs_Selectable	= true;
	filebrowser.Filter		= NULL;//&audiofilefilter;

	hide();

	if (filebrowser.exec(filebrowser.sc_init_dir.c_str()))
	{
#ifdef AUDIOPLAYER_TIME_DEBUG
		timeval start;
		gettimeofday(&start,NULL);
#endif
		CProgressWindow progress;
		long maxProgress = (filebrowser.getSelectedFiles().size() > 1) ? filebrowser.getSelectedFiles().size() - 1: 1;
		long currentProgress = -1;
		//if (maxProgress > SHOW_FILE_LOAD_LIMIT)
		{
			progress.setTitle(LOCALE_AUDIOPLAYER_READING_FILES);
			progress.exec(this,"");
		}

		m_Path = filebrowser.getCurrentDir();
		CFileList::const_iterator files = filebrowser.getSelectedFiles().begin();
		neutrino_msg_t msg;
		neutrino_msg_data_t data;
		g_RCInput->getMsg(&msg, &data, 0);
		for (; (files != filebrowser.getSelectedFiles().end()) && (msg != CRCInput::RC_home); ++files)
		{
			//if (maxProgress > SHOW_FILE_LOAD_LIMIT)
			{
				currentProgress++;
				// show progress
				int global = 100*currentProgress/maxProgress;
				progress.showStatus(global);
				progress.showStatusMessageUTF(files->Name);
#ifdef LCD_UPDATE
				CVFD::getInstance()->showProgressBar(global, "read metadata...");
				CVFD::getInstance()->setMode(CVFD::MODE_PROGRESSBAR);
#endif // LCD_UPDATE
			}
			//printf("processPlaylistUrl(%s, %s)\n", files->Url.c_str(), files->Name.c_str());
			processPlaylistUrl(files->Url.c_str(), files->Name.c_str(), NULL, files->Time);
			g_RCInput->getMsg(&msg, &data, 0);

			if( ( msg>= CRCInput::RC_WithData ) && ( msg< CRCInput::RC_WithData+ 0x10000000 ) )
					delete[] (unsigned char*) data;
		}
		if (m_select_title_by_name)
			buildSearchTree();
#ifdef AUDIOPLAYER_TIME_DEBUG
		timeval end;
		gettimeofday(&end,NULL);
		printf("adding %ld files took: ",maxProgress+1);
		printTimevalDiff(start,end);
#endif
		result = true;
	}
	CScreenSaver::getInstance()->resetIdleTime();
	CVFD::getInstance()->setMode(CVFD::MODE_AUDIO);
	paintLCD();
	// if playlist is turned off -> start playing immediately
	if (!m_show_playlist && !m_playlist.empty())
		play(m_selected);

	return result;
}

void CAudioPlayerGui::hide()
{
	//	printf("hide(){\n");
	if (m_visible)
	{
		clearDetailsLine();
		m_frameBuffer->paintBackgroundBoxRel(m_x, m_y, m_width + OFFSET_SHADOW, m_height + OFFSET_SHADOW);
		m_frameBuffer->blit();
		m_visible = false;
	}
}

void CAudioPlayerGui::paintItem(int pos)
{
	if (!m_show_playlist)
		return;

	int ypos = m_y + m_title_height + OFFSET_SHADOW + OFFSET_INTER + m_header_height + pos*m_item_height;
	unsigned int currpos = m_liststart + pos;

	bool i_selected	= currpos == m_selected;
	bool i_marked	= currpos == (unsigned) m_current;
	bool i_switch	= false; //(currpos < m_playlist.size()) && (pos & 1);
	int i_radius	= RADIUS_NONE;

	fb_pixel_t color;
	fb_pixel_t bgcolor;

	getItemColors(color, bgcolor, i_selected, i_marked, i_switch);

	if (i_selected || i_marked)
		i_radius = RADIUS_LARGE;

	if (i_selected)
		paintDetailsLine(pos);

	if (i_radius)
		m_frameBuffer->paintBoxRel(m_x, ypos, m_width - SCROLLBAR_WIDTH, m_item_height, COL_MENUCONTENT_PLUS_0);
	m_frameBuffer->paintBoxRel(m_x, ypos, m_width - SCROLLBAR_WIDTH, m_item_height, bgcolor, i_radius);

	if (currpos < m_playlist.size())
	{
		char sNr[20];
		sprintf(sNr, (currpos + 1) < 10 ? "%02d: " : "%d: ", currpos + 1);
		std::string tmp = sNr;
		getFileInfoToDisplay(tmp, m_playlist[currpos]);

		char dura[14] = {0};
		if (m_inetmode)
		{
			if (m_playlist[currpos].MetaData.total_time != 0)
				snprintf(dura, sizeof(dura), "%ldk", m_playlist[currpos].MetaData.total_time);
		}
		else
			snprintf(dura, sizeof(dura), "%ld:%02ld", m_playlist[currpos].MetaData.total_time / 60, m_playlist[currpos].MetaData.total_time % 60);

		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(dura);
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x + OFFSET_INNER_MID, ypos + m_item_height, m_width - SCROLLBAR_WIDTH - 3*OFFSET_INNER_MID - w, tmp, color, m_item_height);
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x + m_width - SCROLLBAR_WIDTH - OFFSET_INNER_MID - w, ypos + m_item_height, w, dura, color, m_item_height);
		if (currpos == m_selected)
		{
			if (m_state == CAudioPlayerGui::STOP)
				CVFD::getInstance()->showAudioTrack(m_playlist[currpos].MetaData.artist, m_playlist[currpos].MetaData.title, m_playlist[currpos].MetaData.album);
		}
	}
}

void CAudioPlayerGui::paintHead()
{
	if (!m_show_playlist || CScreenSaver::getInstance()->isActive())
		return;

	CComponentsHeader header(m_x, m_y + m_title_height + OFFSET_SHADOW + OFFSET_INTER, m_width, m_header_height, LOCALE_AUDIOPLAYER_HEAD, NEUTRINO_ICON_AUDIO);
	header.enableShadow( CC_SHADOW_RIGHT | CC_SHADOW_CORNER_TOP_RIGHT | CC_SHADOW_CORNER_BOTTOM_RIGHT, -1, true);

	if (m_inetmode)
		header.setCaption(LOCALE_INETRADIO_NAME);

#ifdef ENABLE_GUI_MOUNT
	if (!m_inetmode)
		header.setContextButton(NEUTRINO_ICON_BUTTON_MENU);
#endif

	header.paint(CC_SAVE_SCREEN_NO);
	m_frameBuffer->blit();
}

	#define BUTTON_LABEL_COUNT 3
	const struct button_label SecondLineButtons[BUTTON_LABEL_COUNT] =
	{
		{ NEUTRINO_ICON_BUTTON_OKAY,	LOCALE_AUDIOPLAYER_PLAY		},
		{ NEUTRINO_ICON_BUTTON_HELP,	LOCALE_AUDIOPLAYER_KEYLEVEL	},
		{ NEUTRINO_ICON_BUTTON_INFO,	LOCALE_PICTUREVIEWER_HEAD	}
	};

void CAudioPlayerGui::paintFoot()
{
	if (CScreenSaver::getInstance()->isActive())
		return;

	int radius = RADIUS_LARGE;
	int button_y = m_y + m_height - OFFSET_SHADOW - m_info_height - OFFSET_INTER - OFFSET_SHADOW - 2*m_button_height;
	int button_x = m_x;
	int button_width = m_width;

	// ensure to get round corners in footer
	if (!m_show_playlist && radius)
	{
		button_x += radius;
		button_width -= 2*radius;
	}

	// shadow
	m_frameBuffer->paintBoxRel(m_x + OFFSET_SHADOW, button_y + OFFSET_SHADOW, m_width, 2*m_button_height, COL_SHADOW_PLUS_0, radius, (m_show_playlist ? CORNER_BOTTOM : CORNER_ALL));
	m_frameBuffer->paintBoxRel(m_x, button_y, m_width, 2*m_button_height, COL_MENUFOOT_PLUS_0, radius, (m_show_playlist ? CORNER_BOTTOM : CORNER_ALL));

	if (!m_playlist.empty())
		::paintButtons(button_x, button_y + m_button_height, button_width, BUTTON_LABEL_COUNT, SecondLineButtons, button_width, m_button_height);

	if (m_key_level == 0)
	{
		if (m_playlist.empty())
		{
			if (m_inetmode)
				::paintButtons(button_x, button_y, button_width, 2, AudioPlayerButtons[7], button_width, m_button_height);
			else
				::paintButtons(button_x, button_y, button_width, 1, &(AudioPlayerButtons[7][0]), button_width, m_button_height);
		}
		else if (m_inetmode)
			::paintButtons(button_x, button_y, button_width, 4, AudioPlayerButtons[8], button_width, m_button_height);
		else
			::paintButtons(button_x, button_y, button_width, 4, AudioPlayerButtons[1], button_width, m_button_height);
	}
	else if (m_key_level == 1)
	{
		if (m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
			::paintButtons(button_x, button_y, button_width, 4, AudioPlayerButtons[0], button_width, m_button_height);
		else
		{
			int b = 2;
			if (m_streamripper_available)
			{
				b = 3;
				if (m_streamripper_active)
					AudioPlayerButtons[6][2].locale = LOCALE_AUDIOPLAYER_STREAMRIPPER_STOP;
				else
					AudioPlayerButtons[6][2].locale = LOCALE_AUDIOPLAYER_STREAMRIPPER_START;
			}
			::paintButtons(button_x, button_y, button_width, b, AudioPlayerButtons[6], button_width, m_button_height);
		}
	}
	else // key_level == 2
	{
		if (m_state == CAudioPlayerGui::STOP)
		{
			if (m_select_title_by_name)
				::paintButtons(button_x, button_y, button_width, 2, AudioPlayerButtons[5], button_width, m_button_height);
			else
				::paintButtons(button_x, button_y, button_width, 2, AudioPlayerButtons[4], button_width, m_button_height);
		}
		else
		{
			if (m_select_title_by_name)
				::paintButtons(button_x, button_y, button_width, 2, AudioPlayerButtons[3], button_width, m_button_height);
			else
				::paintButtons(button_x, button_y, button_width, 2, AudioPlayerButtons[2], button_width, m_button_height);
		}
	}
	m_frameBuffer->blit();
}

void CAudioPlayerGui::paintCover()
{
	const CAudioMetaData meta = CAudioPlayer::getInstance()->getMetaData();

	// try folder.jpg first
	m_cover = m_curr_audiofile.Filename.substr(0, m_curr_audiofile.Filename.rfind('/')) + "/folder.jpg";
	m_stationlogo = false;

	// try cover from tag
	if (!meta.cover.empty())
		m_cover = meta.cover;
	// try station logo
	else if (!meta.logo.empty())
	{
		std::size_t found_url = meta.logo.find("://");
		if (found_url != std::string::npos)
		{
			mkdir(COVERDIR_TMP, 0755);

			std::string filename(meta.logo);
			const size_t last_slash_idx = filename.find_last_of("/");
			if (last_slash_idx != std::string::npos)
				filename.erase(0, last_slash_idx + 1);

			std::string fullname(COVERDIR_TMP);
			fullname += "/" + filename;

			CHTTPTool httptool;
			if (httptool.downloadFile(meta.logo, fullname.c_str()))
			{
				m_cover = fullname;
				m_stationlogo = true;
			}
			else
				m_cover.clear();
		}
	}

	if (access(m_cover.c_str(), F_OK) == 0)
	{
		int cover_x = m_x + OFFSET_INNER_MID;
		int cover_y = m_y + OFFSET_INNER_SMALL;
		m_cover_width = 0;
		CComponentsPicture cover_object(cover_x, cover_y, 0, m_title_height - 2*OFFSET_INNER_SMALL, m_cover);
		cover_object.SetTransparent(CFrameBuffer::TM_BLACK);
		cover_object.setHeight(m_title_height - 2*OFFSET_INNER_SMALL);
		m_cover_width = cover_object.getWidth() + OFFSET_INNER_MID;
		cover_object.paint();
	}
}
void CAudioPlayerGui::getCurrentCaption(std::string* text)
{
	if (m_inetmode)
	{
		*text = m_curr_audiofile.MetaData.album;
	}
	else
	{
		char sNr[20];
		sprintf(sNr, ": %2d", m_current + 1);
		*text = g_Locale->getText(LOCALE_AUDIOPLAYER_PLAYING);
		*text += sNr ;
	}
}

void CAudioPlayerGui::getCurrentTitleArtist(std::string* text)
{
	GetMetaData(m_curr_audiofile);

	if (m_curr_audiofile.MetaData.title.empty())
		*text = m_curr_audiofile.MetaData.artist;
	else if (m_curr_audiofile.MetaData.artist.empty())
		*text = m_curr_audiofile.MetaData.title;
	else if (g_settings.audioplayer_display == TITLE_ARTIST)
	{
		*text = m_curr_audiofile.MetaData.title;
		*text += " - ";
		*text += m_curr_audiofile.MetaData.artist;
	}
	else //if (g_settings.audioplayer_display == ARTIST_TITLE)
	{
		*text = m_curr_audiofile.MetaData.artist;
		*text += " - ";
		*text += m_curr_audiofile.MetaData.title;
	}
}

void CAudioPlayerGui::paintTitleBox()
{
	if (CScreenSaver::getInstance()->isActive())
		return;

	if (m_state == CAudioPlayerGui::STOP && m_show_playlist)
	{
		if (m_titlebox)
		{
			m_titlebox->kill();
			delete m_titlebox; m_titlebox = NULL;
		}
	}
	else
	{
		// title box
		if (!m_titlebox)
		{
			m_titlebox = new CComponentsShapeSquare(m_x, m_y, m_width, m_title_height, NULL, CC_SHADOW_ON);
			m_titlebox->enableFrame(true, FRAME_WIDTH_MIN);
			m_titlebox->setCorner(RADIUS_LARGE);
		}
		m_titlebox->paint(false);
		paintCover();

		// first line (Track number)
		std::string tmp;
		getCurrentCaption(&tmp);

		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp);
		int xstart = (m_width - w)/2;
		if (xstart < OFFSET_INNER_MID + m_cover_width)
			xstart = OFFSET_INNER_MID + m_cover_width;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x + xstart, m_y + OFFSET_INNER_SMALL + 1*m_item_height, m_width - OFFSET_INNER_MID - xstart, tmp, COL_MENUHEAD_TEXT); //caption "current track"

		// second line (Artist/Title...)
		getCurrentTitleArtist(&tmp);

		w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp);
		xstart = (m_width - w)/2;
		if (xstart < OFFSET_INNER_MID + m_cover_width)
			xstart = OFFSET_INNER_MID + m_cover_width;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x + xstart, m_y + OFFSET_INNER_SMALL + 2*m_item_height, m_width - OFFSET_INNER_MID - xstart, tmp, COL_MENUHEAD_TEXT); //artist - title

		// reset so fields get painted always
		m_metainfo.clear();
		m_time_total = 0;
		m_time_played = 0;

		updateMetaData();
		updateTimes(true);
	}
	m_frameBuffer->blit();
}

void CAudioPlayerGui::paint()
{
	if (m_show_playlist)
	{
		unsigned int tmp_max = m_listmaxshow;
		if (!tmp_max)
			tmp_max = 1;
		m_liststart = (m_selected / tmp_max) * m_listmaxshow;
		paintHead();
		for (unsigned int count=0; count<m_listmaxshow; count++)
			paintItem(count);

		// scrollbar
		int total_pages;
		int current_page;
		getScrollBarData(&total_pages, &current_page, m_playlist.size(), m_listmaxshow, m_selected);

		paintScrollBar(m_x + m_width - SCROLLBAR_WIDTH, m_y + m_title_height + OFFSET_SHADOW + OFFSET_INTER + m_header_height, SCROLLBAR_WIDTH, m_item_height*m_listmaxshow, total_pages, current_page, CC_SHADOW_ON);
	}

	paintTitleBox();
	paintFoot();
	m_frameBuffer->blit();
	m_visible = true;
}

void CAudioPlayerGui::clearDetailsLine()
{
	paintDetailsLine(-1);
}

void CAudioPlayerGui::paintDetailsLine(int pos)
{
	int xpos = m_x - DETAILSLINE_WIDTH;
	int ypos1 = m_y + m_title_height + OFFSET_SHADOW + OFFSET_INTER + m_header_height + pos*m_item_height;
	int ypos2 = m_y + (m_height - OFFSET_SHADOW - m_info_height);
	int ypos1a = ypos1 + (m_item_height / 2);
	int ypos2a = ypos2 + (m_info_height / 2);

	// clear details line
	if (m_detailsline != NULL)
	{
		m_detailsline->kill();
		delete m_detailsline;
		m_detailsline = NULL;
	}

	// paint Line if detail info (and not valid list pos) and info box
	if (!m_playlist.empty() && (pos >= 0))
	{
		//details line
		if (m_detailsline == NULL)
			m_detailsline = new CComponentsDetailsLine(xpos, ypos1a, ypos2a, m_item_height/2, m_info_height - RADIUS_LARGE*2);
		m_detailsline->paint(false);

		// paint id3 infobox
		if (m_infobox == NULL)
		{
			m_infobox = new CComponentsInfoBox(m_x, ypos2, m_width, m_info_height);
			m_infobox->setFrameThickness(FRAME_WIDTH_MIN);
			m_infobox->setCorner(RADIUS_LARGE);
			m_infobox->setColorFrame(COL_FRAME_PLUS_0);
			m_infobox->setColorBody(COL_MENUCONTENTDARK_PLUS_0);
			m_infobox->enableShadow(CC_SHADOW_ON, -1, true);
			m_infobox->forceTextPaint(false);
		}

		// first line
		std::string text_info("");
		getFileInfoToDisplay(text_info, m_playlist[m_selected]);

		text_info += "\n";

		// second line; url or date and genre
		if (m_inetmode)
		{
			if (!m_playlist[m_selected].MetaData.url.empty())
			{
				text_info += m_playlist[m_selected].MetaData.url;
			}
			else
				text_info += " ";
		}
		else
		{
			bool got_date = false;
			if (!m_playlist[m_selected].MetaData.date.empty())
			{
				got_date = true;
				text_info += m_playlist[m_selected].MetaData.date;
			}

			bool got_genre = false;
			if (!m_playlist[m_selected].MetaData.genre.empty())
			{
				got_genre = true;
				if (got_date)
					text_info += " / ";
				text_info += m_playlist[m_selected].MetaData.genre;
			}

			if (!got_date && !got_genre)
				text_info += " ";
		}

		m_infobox->setText(text_info, CTextBox::AUTO_WIDTH, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO], COL_MENUCONTENTDARK_TEXT);
		m_infobox->paint(false);
	}
	else
	{
		if (m_detailsline != NULL)
			m_detailsline->kill();
		if (m_infobox != NULL)
			m_infobox->kill();
	}
	m_frameBuffer->blit();
}

void CAudioPlayerGui::stop()
{
	m_state = CAudioPlayerGui::STOP;
	m_current = 0;

	if (!pictureviewer)
	{
		//LCD
		paintLCD();
		//Display
		paintTitleBox();
		m_key_level = 0;
		paintFoot();
	}

	if (CAudioPlayer::getInstance()->getState() != CBaseDec::STOP)
		CAudioPlayer::getInstance()->stop();

	cleanupCovers();

	if (m_streamripper_active)
	{
		ShowHint(LOCALE_MESSAGEBOX_INFO, LOCALE_AUDIOPLAYER_STREAMRIPPER_STOP, HINTBOX_MIN_WIDTH, 2);
		my_system(2, "streamripper.sh", "stop");
		m_streamripper_active = false;
	}
}

void CAudioPlayerGui::pause()
{
	if (
		   m_state == CAudioPlayerGui::PLAY
		|| m_state == CAudioPlayerGui::FF
		|| m_state == CAudioPlayerGui::REV
	)
	{
		m_state = CAudioPlayerGui::PAUSE;
		CAudioPlayer::getInstance()->pause();
	}
	else if (m_state == CAudioPlayerGui::PAUSE)
	{
		m_state = CAudioPlayerGui::PLAY;
		CAudioPlayer::getInstance()->pause();
	}
	paintLCD();
}

void CAudioPlayerGui::ff(unsigned int seconds)
{
	if (m_state == CAudioPlayerGui::FF)
	{
		m_state = CAudioPlayerGui::PLAY;
		CAudioPlayer::getInstance()->ff(seconds);
	}
	else if (
		   m_state == CAudioPlayerGui::PLAY
		|| m_state == CAudioPlayerGui::PAUSE
		|| m_state == CAudioPlayerGui::REV
	)
	{
		m_state = CAudioPlayerGui::FF;
		CAudioPlayer::getInstance()->ff(seconds);
	}
	paintLCD();
}

void CAudioPlayerGui::rev(unsigned int seconds)
{
	if (m_state == CAudioPlayerGui::REV)
	{
		m_state = CAudioPlayerGui::PLAY;
		CAudioPlayer::getInstance()->rev(seconds);
	}
	else if (
		   m_state == CAudioPlayerGui::PLAY
		|| m_state == CAudioPlayerGui::PAUSE
		|| m_state == CAudioPlayerGui::FF
	)
	{
		m_state = CAudioPlayerGui::REV;
		CAudioPlayer::getInstance()->rev(seconds);
	}
	paintLCD();
}

void CAudioPlayerGui::play(unsigned int pos)
{
	//printf("AudioPlaylist: play %d/%d\n",pos,playlist.size());
	unsigned int old_current = m_current;
	unsigned int old_selected = m_selected;

	cleanupCovers();

	m_current = pos;
	if (g_settings.audioplayer_follow)
		m_selected = pos;

	if (m_selected - m_liststart >= m_listmaxshow && g_settings.audioplayer_follow)
	{
		m_liststart = m_selected;
		if (!CScreenSaver::getInstance()->isActive() && !pictureviewer)
			paint();
	}
	else if (m_liststart < m_selected && g_settings.audioplayer_follow)
	{
		m_liststart = m_selected - m_listmaxshow + 1;
		if (!CScreenSaver::getInstance()->isActive() && !pictureviewer)
			paint();
	}
	else
	{
		if (old_current >= m_liststart && old_current - m_liststart < m_listmaxshow)
		{
			if (!CScreenSaver::getInstance()->isActive() && !pictureviewer)
				paintItem(old_current - m_liststart);
		}
		if (pos >= m_liststart && pos - m_liststart < m_listmaxshow)
		{
			if (!CScreenSaver::getInstance()->isActive() && !pictureviewer)
				paintItem(pos - m_liststart);
		}
		if (g_settings.audioplayer_follow)
		{
			if (old_selected >= m_liststart && old_selected - m_liststart < m_listmaxshow)
				if (!CScreenSaver::getInstance()->isActive() && !pictureviewer)
					paintItem(old_selected - m_liststart);
		}
	}

	GetMetaData(m_playlist[pos]);

	m_metainfo.clear();
	m_time_played = 0;
	m_time_total = m_playlist[m_current].MetaData.total_time;
	m_state = CAudioPlayerGui::PLAY;
	m_curr_audiofile = m_playlist[m_current];

	if (CScreenSaver::getInstance()->isActive() && g_settings.audioplayer_cover_as_screensaver)
		CScreenSaver::getInstance()->forceRefresh();

	// Play
	CAudioPlayer::getInstance()->play(&m_curr_audiofile, g_settings.audioplayer_highprio == 1);

	if (!pictureviewer)
	{
		//LCD
		paintLCD();
		// Display
		paintTitleBox();
		m_key_level = 1;
		paintFoot();
	}
}

int CAudioPlayerGui::getNext()
{
	int ret= m_current + 1;
	if (m_playlist.empty())
		return -1;
	if ((unsigned)ret >= m_playlist.size())
	{
		if (g_settings.audioplayer_repeat_on == 1)
			ret = 0;
		else
			ret = -1;
	}
	return ret;
}

void CAudioPlayerGui::updateMetaData()
{
	bool updateMeta = false;
	bool updateLcd = false;
	bool updateScreen = false;

	if (m_state == CAudioPlayerGui::STOP)
		return;

	if (CAudioPlayer::getInstance()->hasMetaDataChanged() || m_metainfo.empty())
	{
		const CAudioMetaData meta = CAudioPlayer::getInstance()->getMetaData();

		std::stringstream info;
		info.precision(3);

		if (meta.bitrate > 0)
		{
			info << " / ";
			if (meta.vbr)
				info << "VBR ";
			info << meta.bitrate/1000 << "kbps";
		}

		if (meta.samplerate > 0)
			info << " / " << meta.samplerate/1000 << "." << (meta.samplerate/100)%10 <<"kHz";

		m_metainfo = meta.type_info + info.str();
		updateMeta = !CScreenSaver::getInstance()->isActive();

		if (!meta.artist.empty() && meta.artist != m_curr_audiofile.MetaData.artist)
		{
			m_curr_audiofile.MetaData.artist = meta.artist;

			if (!CScreenSaver::getInstance()->isActive())
				updateScreen = true;
			updateLcd = true;
		}

		if (!meta.title.empty() && meta.title != m_curr_audiofile.MetaData.title)
		{
			m_curr_audiofile.MetaData.title = meta.title;

			if (!CScreenSaver::getInstance()->isActive())
				updateScreen = true;
			updateLcd = true;
		}

		if (!meta.sc_station.empty() && meta.sc_station != m_curr_audiofile.MetaData.album)
		{
			m_curr_audiofile.MetaData.album = meta.sc_station;
			updateLcd = true;
		}

		if (!CScreenSaver::getInstance()->isActive())
			paintCover();
	}
	if (CAudioPlayer::getInstance()->hasMetaDataChanged() != 0)
		updateLcd = true;

	//printf("CAudioPlayerGui::updateMetaData: updateLcd %d\n", updateLcd);
	if (updateLcd)
		paintLCD();

	if (updateScreen)
		paintTitleBox();

	if (updateMeta || updateScreen)
	{
		m_frameBuffer->paintBoxRel(m_x + OFFSET_INNER_MID + m_cover_width, m_y + OFFSET_INNER_SMALL + 2*m_item_height + OFFSET_INNER_SMALL, m_width - 2*OFFSET_INNER_MID - m_cover_width, m_meta_height, m_titlebox->getColorBody());

		int w = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(m_metainfo);
		int xstart = (m_width - w)/2;
		if (xstart < OFFSET_INNER_MID)
			xstart = OFFSET_INNER_MID;
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(m_x + xstart, m_y + OFFSET_INNER_SMALL + 2*m_item_height + OFFSET_INNER_SMALL + m_meta_height, m_width - 2*xstart, m_metainfo, COL_MENUHEAD_TEXT);
		m_frameBuffer->blit();
	}
}

void CAudioPlayerGui::updateTimes(const bool force)
{
	if (m_state != CAudioPlayerGui::STOP)
	{
		bool updateTotal = force;
		bool updatePlayed = force;

		if (m_time_total != CAudioPlayer::getInstance()->getTimeTotal())
		{
			m_time_total = CAudioPlayer::getInstance()->getTimeTotal();
			if (m_curr_audiofile.MetaData.total_time != CAudioPlayer::getInstance()->getTimeTotal())
			{
				m_curr_audiofile.MetaData.total_time = CAudioPlayer::getInstance()->getTimeTotal();
				if (m_current >= 0)
					m_playlist[m_current].MetaData.total_time = CAudioPlayer::getInstance()->getTimeTotal();
			}
			updateTotal = true;
		}
		if ((m_time_played != CAudioPlayer::getInstance()->getTimePlayed()))
		{
			m_time_played = CAudioPlayer::getInstance()->getTimePlayed();
			updatePlayed = true;
		}
		if (!CScreenSaver::getInstance()->isActive())
		{
			char total_time[17];
			snprintf(total_time, sizeof(total_time), " / %ld:%02ld", m_time_total / 60, m_time_total % 60);
			char played_time[14];
			snprintf(played_time, sizeof(played_time), "%ld:%02ld", m_time_played / 60, m_time_played % 60);

			int w_total_time = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(total_time);
			int w_faked_time = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth("000:00");
			int w_played_time = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(played_time);

			// in inetmode we havn't a total_time
			if (m_inetmode)
				w_total_time = 0;

			int x_total_time = m_x + m_width - OFFSET_INNER_MID - w_total_time - 2*m_titlebox->getFrameThickness();
			// played time offset to align this value on the right side
			int o_played_time = std::max(w_faked_time - w_played_time, 0);
			int x_faked_time = x_total_time - w_faked_time;
			int x_played_time = x_faked_time + o_played_time;
			int y_times = m_y + OFFSET_INNER_SMALL;

			if (updateTotal && !m_inetmode)
			{
				m_frameBuffer->paintBoxRel(x_total_time, y_times, w_total_time, m_item_height, m_titlebox->getColorBody());
				if (m_time_total > 0)
				{
					g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x_total_time, y_times + m_item_height, w_total_time, total_time, COL_MENUHEAD_TEXT); //total time
				}
			}
			if (updatePlayed || (m_state == CAudioPlayerGui::PAUSE))
			{
				m_frameBuffer->paintBoxRel(x_faked_time, y_times, w_faked_time, m_item_height, m_titlebox->getColorBody());
				struct timeval tv;
				gettimeofday(&tv, NULL);
				if ((m_state != CAudioPlayerGui::PAUSE) || (tv.tv_sec & 1))
				{
					g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x_played_time, y_times + m_item_height, w_played_time, played_time, COL_MENUHEAD_TEXT); //elapsed time
				}
			}
			m_frameBuffer->blit();
		}
		if ((updatePlayed || updateTotal) && m_curr_audiofile.FileType != CFile::STREAM_AUDIO && m_time_total != 0)
		{
			CVFD::getInstance()->showAudioProgress(uint8_t(100 * m_time_played / m_time_total));
		}
#ifdef ENABLE_GRAPHLCD
			//glcd_duration = to_string(m_time_played / (60 * 1000)) + "/" + to_string(m_time_total / (60 * 1000));
			glcd_duration = std::to_string(m_time_played/60) + ":" + std::to_string(m_time_played%60) + "/" + to_string(m_time_total/60) + ":" + to_string(m_time_total%60);

			time_t sTime = time(NULL);
			sTime -= m_time_played;
			tm_struct = localtime(&sTime);
			glcd_start = std::to_string(tm_struct->tm_hour/10) + std::to_string(tm_struct->tm_hour%10) + ":" + to_string(tm_struct->tm_min/10) + to_string(tm_struct->tm_min%10);

			time_t eTime = time(NULL);
			eTime += m_time_total - m_time_played;
			tm_struct = localtime(&eTime);
			glcd_end = std::to_string(tm_struct->tm_hour/10) + std::to_string(tm_struct->tm_hour%10) + ":" + to_string(tm_struct->tm_min/10) + to_string(tm_struct->tm_min%10);

			glcd_position = 100 * m_time_played / m_time_total;

			cGLCD::lockChannel(glcd_channel, glcd_epg, uint8_t(glcd_position));
			cGLCD::lockDuration(glcd_duration);
			cGLCD::lockStart(glcd_start);
			cGLCD::lockEnd(glcd_end);
#endif
	}
}

void CAudioPlayerGui::paintLCD()
{
#ifdef ENABLE_GRAPHLCD
	const CAudioMetaData meta = CAudioPlayer::getInstance()->getMetaData();
	if ( !meta.artist.empty() )
		glcd_epg = meta.artist;
	if ( !meta.artist.empty() && !meta.title.empty() )
		glcd_epg = " - ";
	if ( !meta.title.empty() )
		glcd_epg = meta.title;
#endif
	switch (m_state)
	{
		case CAudioPlayerGui::STOP:
			CVFD::getInstance()->showAudioPlayMode(CVFD::AUDIO_MODE_STOP);
			CVFD::getInstance()->showAudioProgress(0);
#ifdef ENABLE_GRAPHLCD
			if (m_inetmode)
				glcd_channel = g_Locale->getText(LOCALE_INETRADIO_NAME);
			else
				glcd_channel = g_Locale->getText(LOCALE_AUDIOPLAYER_NAME);

			glcd_epg = g_Locale->getText(LOCALE_MPKEY_STOP);

			cGLCD::ShowLcdIcon(false);
			cGLCD::lockChannel(glcd_channel, glcd_epg, 0);
			cGLCD::lockDuration("00/00");
			cGLCD::lockStart("00:00");
			cGLCD::lockEnd("00:00");
#endif
			break;
		case CAudioPlayerGui::PLAY:
#ifdef ENABLE_GRAPHLCD
			glcd_channel = "";
			cGLCD::lockChannel(glcd_channel, glcd_epg, uint8_t(glcd_position));
			cGLCD::lockDuration(glcd_duration);
			cGLCD::lockStart(glcd_start);
			cGLCD::lockEnd(glcd_end);
			cGLCD::ShowLcdIcon(true);
#endif
			CVFD::getInstance()->showAudioPlayMode(CVFD::AUDIO_MODE_PLAY);
			CVFD::getInstance()->showAudioTrack(m_curr_audiofile.MetaData.artist, m_curr_audiofile.MetaData.title, m_curr_audiofile.MetaData.album);
			if (m_curr_audiofile.FileType != CFile::STREAM_AUDIO && m_time_total != 0)
				CVFD::getInstance()->showAudioProgress(uint8_t(100 * m_time_played / m_time_total));
			break;
		case CAudioPlayerGui::PAUSE:
			CVFD::getInstance()->showAudioPlayMode(CVFD::AUDIO_MODE_PAUSE);
			CVFD::getInstance()->showAudioTrack(m_curr_audiofile.MetaData.artist, m_curr_audiofile.MetaData.title, m_curr_audiofile.MetaData.album);
#ifdef ENABLE_GRAPHLCD
			glcd_channel = "";
			cGLCD::lockChannel(glcd_channel, glcd_epg, uint8_t(glcd_position));
			cGLCD::lockDuration(glcd_duration);
			cGLCD::lockStart(glcd_start);
			cGLCD::lockEnd(glcd_end);
			cGLCD::ShowLcdIcon(true);
#endif
			break;
		case CAudioPlayerGui::FF:
			CVFD::getInstance()->showAudioPlayMode(CVFD::AUDIO_MODE_FF);
			CVFD::getInstance()->showAudioTrack(m_curr_audiofile.MetaData.artist, m_curr_audiofile.MetaData.title, m_curr_audiofile.MetaData.album);
#ifdef ENABLE_GRAPHLCD
			glcd_channel = "";
			cGLCD::lockChannel(glcd_channel, glcd_epg, uint8_t(glcd_position));
			cGLCD::lockDuration(glcd_duration);
			cGLCD::lockStart(glcd_start);
			cGLCD::lockEnd(glcd_end);
			cGLCD::ShowLcdIcon(true);
#endif
			break;
		case CAudioPlayerGui::REV:
			CVFD::getInstance()->showAudioPlayMode(CVFD::AUDIO_MODE_REV);
			CVFD::getInstance()->showAudioTrack(m_curr_audiofile.MetaData.artist, m_curr_audiofile.MetaData.title, m_curr_audiofile.MetaData.album);
#ifdef ENABLE_GRAPHLCD
			glcd_channel = "";
			cGLCD::lockChannel(glcd_channel, glcd_epg, uint8_t(glcd_position));
			cGLCD::lockDuration(glcd_duration);
			cGLCD::lockStart(glcd_start);
			cGLCD::lockEnd(glcd_end);
			cGLCD::ShowLcdIcon(true);
#endif
			break;
	}
}

void CAudioPlayerGui::GetMetaData(CAudiofileExt &File)
{
	bool ret = 1;

	if (File.FileType != CFile::STREAM_AUDIO && !File.MetaData.bitrate)
		ret = CAudioPlayer::getInstance()->readMetaData(&File, m_state != CAudioPlayerGui::STOP && !g_settings.audioplayer_highprio);

	if (!ret || (File.MetaData.artist.empty() && File.MetaData.title.empty()))
	{
		//Set from Filename
		std::string tmp = File.Filename.substr(File.Filename.rfind('/') + 1);
		tmp = tmp.substr(0,tmp.length() - 4); //remove extension (.mp3)
		std::string::size_type i = tmp.rfind(" - ");
		if (i != std::string::npos)
		{ // Trennzeichen " - " gefunden
			File.MetaData.artist = tmp.substr(0, i);
			File.MetaData.title = tmp.substr(i + 3);
		}
		else
		{
			i = tmp.rfind('-');
			if (i != std::string::npos)
			{ //Trennzeichen "-"
				File.MetaData.artist = tmp.substr(0, i);
				File.MetaData.title = tmp.substr(i + 1);
			}
			else
				File.MetaData.title = tmp;
		}
		File.MetaData.artist = FILESYSTEM_ENCODING_TO_UTF8_STRING(File.MetaData.artist);
		File.MetaData.title  = FILESYSTEM_ENCODING_TO_UTF8_STRING(File.MetaData.title);
	}
}

bool CAudioPlayerGui::getNumericInput(neutrino_msg_t& msg, int& val)
{
	//FIXME - remove fixed values

	neutrino_msg_data_t data;
	int x1 = (g_settings.screen_EndX - g_settings.screen_StartX) / 2 + g_settings.screen_StartX - 50;
	int y1 = (g_settings.screen_EndY - g_settings.screen_StartY) / 2 + g_settings.screen_StartY;
	char str[11];
	do
	{

		val = val * 10 + CRCInput::getNumericValue(msg);
		sprintf(str, "%d", val);
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getRenderWidth(str);
		int h = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getHeight();
		m_frameBuffer->paintBoxRel(x1 - 7, y1 - h - 5, w + 14, h + 10, COL_FRAME_PLUS_0);
		m_frameBuffer->paintBoxRel(x1 - 6, y1 - h - 4, w + 12, h +  8, COL_MENUCONTENTSELECTED_PLUS_0);
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->RenderString(x1, y1, w + 1, str, COL_MENUCONTENTSELECTED_TEXT);
		m_frameBuffer->blit();
		while (true)
		{
			g_RCInput->getMsg(&msg, &data, 100);
			if (msg > CRCInput::RC_MaxRC && msg != CRCInput::RC_timeout)
			{	// not a key event
				CNeutrinoApp::getInstance()->handleMsg(msg, data);
				continue;
			}
			if (msg & (CRCInput::RC_Repeat|CRCInput::RC_Release)) // repeat / release
				continue;
			break;
		}
	} while (g_RCInput->isNumeric(msg) && val < 1000000);
	return (msg == CRCInput::RC_ok);
}

void CAudioPlayerGui::getFileInfoToDisplay(std::string &fileInfo, CAudiofileExt &file)
{
	//std::string fileInfo;
	std::string artist;
	std::string title;

	if (!m_inetmode)
	{
		artist ="Artist?";
		title = "Title?";
	}

	GetMetaData(file);

	if (!file.MetaData.artist.empty())
		artist = file.MetaData.artist;

	if (!file.MetaData.title.empty())
		title = file.MetaData.title;

	if (g_settings.audioplayer_display == TITLE_ARTIST)
	{
		fileInfo += title;
		if (!title.empty() && !artist.empty())
			fileInfo += " - ";
		fileInfo += artist;
	}
	else //if (g_settings.audioplayer_display == ARTIST_TITLE)
	{
		fileInfo += artist;
		if (!title.empty() && !artist.empty())
			fileInfo += " - ";
		fileInfo += title;
	}

	if (!file.MetaData.album.empty())
	{
		fileInfo += " - ";
		fileInfo += file.MetaData.album;
	}

	if (fileInfo.empty())
	{
		fileInfo += "Unknown";
	}

	file.firstChar = (char)tolower(fileInfo[0]);
	//info += fileInfo;
}

void CAudioPlayerGui::addToPlaylist(CAudiofileExt &file)
{
	//printf("add2Playlist: %s\n", file.Filename.c_str());
	if (m_select_title_by_name)
	{
		std::string t("");
		getFileInfoToDisplay(t,file);
	}
	else
		GetMetaData(file);

	m_playlist.push_back(file);
	m_playlistHasChanged = true;
}

void CAudioPlayerGui::removeFromPlaylist(long pos)
{
	unsigned char firstChar = ' ';
	// must be called before m_playlist.erase()
	if (m_select_title_by_name)
		firstChar = getFirstChar(m_playlist[pos]);

	//unlink(m_playlist[pos].MetaData.cover.c_str());
	m_playlist.erase(m_playlist.begin() + pos);
	m_playlistHasChanged = true;

	if (m_select_title_by_name)
	{
#ifdef AUDIOPLAYER_TIME_DEBUG
		timeval start;
		gettimeofday(&start,NULL);
#endif
		//printf("searching for key: %c val: %ld\n",firstChar,pos);

		CTitle2Pos::iterator item = m_title2Pos.find(firstChar);
		if (item != m_title2Pos.end())
		{
			item->second.erase(pos);

			// delete empty entries
			if (item->second.empty())
				m_title2Pos.erase(item);
		}
		else
		{
			printf("could not find key: %c pos: %ld\n",firstChar,pos);
		}
		// decrease position information for all titles with a position
		// behind item to delete
		long p = 0;
		for (CTitle2Pos::iterator title=m_title2Pos.begin(); title!=m_title2Pos.end(); ++title)
		{
			CPosList newList;
			for (CPosList::iterator posIt=title->second.begin(); posIt!=title->second.end(); ++posIt)
			{
				p = *(posIt);
				if (*posIt > pos)
					p--;
				// old list is sorted so we can give a hint to insert at the end
				newList.insert(newList.end(), p);
			}
			title->second = newList;
		}
#ifdef AUDIOPLAYER_TIME_DEBUG
		timeval end;
		gettimeofday(&end,NULL);
		printf("delete took: ");
		printTimevalDiff(start,end);
#endif
	}
}

void CAudioPlayerGui::selectTitle(unsigned char selectionChar)
{
	unsigned long i;

	//printf("fastLookup: key %c\n",selectionChar);
	CTitle2Pos::iterator it = m_title2Pos.find(selectionChar);
	if (it!=m_title2Pos.end())
	{
		// search for the next greater id
		// if nothing found take the first
		CPosList::iterator posIt = it->second.upper_bound(m_selected);
		if (posIt != it->second.end())
		{
			i = *posIt;
			//printf("upper bound i: %ld\n",i);
		}
		else
		{
			if (!it->second.empty())
			{
				i = *(it->second.begin());
				//printf("using begin i: %ld\n",i);
			}
			else
			{
				//printf("no title with that key\n");
				return;
			}
		}
	}
	else
	{
		//printf("no title with that key\n");
		return;
	}

	int prevselected = m_selected;
	m_selected = i;

	unsigned int oldliststart = m_liststart;
	m_liststart = (m_selected / m_listmaxshow)*m_listmaxshow;
	//printf("before paint\n");
	if (oldliststart != m_liststart)
	{
		paint();
	}
	else
	{
		paintItem(prevselected - m_liststart);
		paintItem(m_selected - m_liststart);
	}
}

void CAudioPlayerGui::printSearchTree()
{
	for (CTitle2Pos::iterator it=m_title2Pos.begin(); it!=m_title2Pos.end(); ++it)
	{
		printf("key: %c\n",it->first);
		long pos=-1;
		for (CPosList::iterator it2=it->second.begin(); it2!=it->second.end(); ++it2)
		{
			pos++;
			printf(" val: %ld ",*it2);
			if (pos % 5 == 4)
				printf("\n");
		}
		printf("\n");
	}
}

void CAudioPlayerGui::buildSearchTree()
{
	//printf("before\n");
	//printSearchTree();

#ifdef AUDIOPLAYER_TIME_DEBUG
	timeval start;
	gettimeofday(&start,NULL);
#endif

	CProgressWindow progress;
	progress.setTitle(LOCALE_AUDIOPLAYER_BUILDING_SEARCH_INDEX);
	progress.exec(this, "");

	long maxProgress = (m_playlist.size() > 1) ? m_playlist.size() - 1 : 1;

	m_title2Pos.clear();
	long listPos = -1;

	for (CAudioPlayList::iterator it=m_playlist.begin(); it!=m_playlist.end(); ++it)
	{
		listPos++;
		progress.showStatus(100*listPos / maxProgress);
		progress.showStatusMessageUTF(it->Filename);
		unsigned char firstChar = getFirstChar(*it);
		const std::pair<CTitle2Pos::iterator,bool> item = m_title2Pos.insert(CTitle2PosItem(firstChar,CPosList()));
		item.first->second.insert(listPos);
	}
	progress.hide();
	m_playlistHasChanged = false;

#ifdef AUDIOPLAYER_TIME_DEBUG
	timeval end;
	gettimeofday(&end,NULL);
	printf("searchtree took: ");
	printTimevalDiff(start,end);
#endif
	//printf("after:\n");
	//printSearchTree();
}

unsigned char CAudioPlayerGui::getFirstChar(CAudiofileExt &file)
{
	if (file.firstChar == '\0')
	{
		std::string info("");
		getFileInfoToDisplay(info, file);
	}
	//printf("getFirstChar: %c\n",file.firstChar);
	return file.firstChar;
}

#ifdef AUDIOPLAYER_TIME_DEBUG
void CAudioPlayerGui::printTimevalDiff(timeval &start, timeval &end)
{
	long secs = end.tv_sec - start.tv_sec;
	long usecs = end.tv_usec -start.tv_usec;
	if (usecs < 0)
	{
		usecs = 1000000 + usecs;
		secs--;
	}
	printf("%ld:%ld\n",secs,usecs);
}
#endif

void CAudioPlayerGui::savePlaylist()
{
	const char *path;

	// .m3u playlist
	// http://hanna.pyxidis.org/tech/m3u.html

	CFileBrowser browser;
	browser.Multi_Select = false;
	browser.Dir_Mode = true;
	CFileFilter dirFilter;
	dirFilter.addFilter("m3u");
	browser.Filter = &dirFilter;
	// select preferred directory if exists
	if (!g_settings.network_nfs_audioplayerdir.empty())
		path = g_settings.network_nfs_audioplayerdir.c_str();
	else
		path = "/";

	// let user select target directory
	this->hide();
	if (browser.exec(path))
	{
		// refresh view
		this->paint();
		CFile *file = browser.getSelectedFile();
		std::string absPlaylistDir = file->getPath();

		// add a trailing slash if necessary
		if ((absPlaylistDir.empty()) || ((*(absPlaylistDir.rbegin()) != '/')))
			absPlaylistDir += '/';

		absPlaylistDir += file->getFileName();

		const int filenamesize = 30;
		std::string filename = "playlist";

		if (file->getType() == CFile::FILE_PLAYLIST)
		{
			// file is playlist so we should ask if we can overwrite it
			std::string name = file->getPath();
			name += '/';
			name += file->getFileName();
			bool overwrite = askToOverwriteFile(name);
			if (!overwrite)
				return;
			filename = name;
		}
		else if (file->getType() == CFile::FILE_DIR)
		{
			// query for filename
			this->hide();
			CKeyboardInput filenameInput(LOCALE_AUDIOPLAYER_PLAYLIST_NAME, &filename, filenamesize - 1, NULL, NULL, LOCALE_AUDIOPLAYER_PLAYLIST_NAME_HINT1, LOCALE_AUDIOPLAYER_PLAYLIST_NAME_HINT2);
			filenameInput.exec(NULL, "");
			// refresh view
			this->paint();
			std::string name = absPlaylistDir;
			name += '/';
			name += filename;
			name += ".m3u";
			std::ifstream input(name.c_str());

			// test if file exists and query for overwriting it or not
			if (input.is_open())
			{
				bool overwrite = askToOverwriteFile(name);
				if (!overwrite)
					return;
			}
			input.close();
		}
		else
		{
			std::cout << "CAudioPlayerGui: neither .m3u nor directory selected, abort" << std::endl;
			return;
		}
		std::string absPlaylistFilename = absPlaylistDir;
		absPlaylistFilename += '/';
		absPlaylistFilename += filename;
		absPlaylistFilename += ".m3u";
		std::ofstream playlistFile(absPlaylistFilename.c_str());
		std::cout << "CAudioPlayerGui: writing playlist to " << absPlaylistFilename << std::endl;
		if (!playlistFile)
		{
			// an error occured
			const int msgsize = 255;
			char msg[msgsize] = "";
			snprintf(msg, msgsize,
					"%s\n%s",
					g_Locale->getText(LOCALE_AUDIOPLAYER_PLAYLIST_FILEERROR_MSG),
			absPlaylistFilename.c_str());

			DisplayErrorMessage(msg);
			// refresh view
			this->paint();
			std::cout << "CAudioPlayerGui: could not create play list file " << absPlaylistFilename << std::endl;
			return;
		}
		// writing .m3u file
		playlistFile << "#EXTM3U" << std::endl;

		CAudioPlayList::const_iterator it;
		for (it = m_playlist.begin(); it!=m_playlist.end(); ++it)
		{
			playlistFile << "#EXTINF:" << it->MetaData.total_time << "," << it->MetaData.artist << " - " << it->MetaData.title << std::endl;
			if (m_inetmode)
				playlistFile << it->Filename << std::endl;
			else
				playlistFile << absPath2Rel(absPlaylistDir, it->Filename) << std::endl;
		}
		playlistFile.close();
	}
	this->paint();
}

bool CAudioPlayerGui::askToOverwriteFile(const std::string& filename)
{

	char msg[filename.length() + 127];
	snprintf(msg, filename.length() + 126,
			"%s\n%s",
			g_Locale->getText(LOCALE_AUDIOPLAYER_PLAYLIST_FILEOVERWRITE_MSG),
	filename.c_str());
	bool res = (ShowMsg(LOCALE_AUDIOPLAYER_PLAYLIST_FILEOVERWRITE_TITLE, msg, CMsgBox::mbrYes, CMsgBox::mbYes | CMsgBox::mbNo) == CMsgBox::mbrYes);
	this->paint();
	return res;
}

std::string CAudioPlayerGui::absPath2Rel(const std::string& fromDir, const std::string& absFilename)
{
	std::string res = "";

	int length = fromDir.length() < absFilename.length() ? fromDir.length() : absFilename.length();
	int lastSlash = 0;
	// find common prefix for both paths
	// fromDir:	/foo/bar/angle/1		(length: 16)
	// absFilename:	/foo/bar/devil/2/fire.mp3	(length: 19)
	// -> /foo/bar/ is prefix, lastSlash will be 8
	for (int i=0; i<length; i++)
	{
		if (fromDir[i] == absFilename[i])
		{
			if (fromDir[i] == '/')
				lastSlash = i;
		}
		else
		{
			break;
		}
	}
	// cut common prefix
	std::string relFilepath = absFilename.substr(lastSlash + 1, absFilename.length() - lastSlash + 1);
	// relFilepath is now devil/2/fire.mp3

	// First slash is not removed because we have to go up each directory.
	// Since the slashes are counted later we make sure for each directory one slash is present
	std::string relFromDir = fromDir.substr(lastSlash, fromDir.length() - lastSlash);
	// relFromDir is now /angle/1

	// go up as many directories as neccessary
	for (unsigned int i=0; i<relFromDir.size(); i++)
	{
		if (relFromDir[i] == '/')
			res = res + "../";
	}

	res = res + relFilepath;
	return res;
}

void CAudioPlayerGui::cleanupCovers()
{
	if (access(COVERDIR_TMP, F_OK) == 0)
	{
		struct dirent **coverlist;
		int n = scandir(COVERDIR_TMP, &coverlist, 0, alphasort);
		if (n > -1)
		{
			while (n--)
			{
				const char *coverfile = coverlist[n]->d_name;
				if (strcmp(coverfile, ".") != 0 && strcmp(coverfile, "..") != 0)
				{
					printf("[audioplayer] removing cover %s/%s\n", COVERDIR_TMP, coverfile);
					unlink(((std::string)COVERDIR_TMP + "/" + coverfile).c_str());
				}
				free(coverlist[n]);
			}
			free(coverlist);
		}
	}

	m_cover.clear();
	m_stationlogo = false;
}
