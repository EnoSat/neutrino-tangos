
/***************************************************************************
	Neutrino-GUI  -   DBoxII-Project

	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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

	***********************************************************

	Module Name: listframe.h .

	Description: interface of the CTextBox class

	Date:	Nov 2005

	Author: Günther@tuxbox.berlios.org
		based on code of Steffen Hehn 'McClean'

	Revision History:
	Date			Author		Change Description
	   Nov 2005		Günther	initial implementation
****************************************************************************/

#ifndef __LISTFRAME_H__
#define __LISTFRAME_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>
#include <gui/widget/textbox.h>
#include <driver/fb_window.h>
#include <gui/components/cc.h>

#define LF_MAX_ROWS 10

typedef struct lf_row_types_t
{
	std::vector<std::string> v_text;
	std::vector<CComponentsItem*> v_ccItem;
}lf_row_types_struct;

typedef struct lf_line_types_t
{
	int rows;
	std::string lineHeader[LF_MAX_ROWS];
	lf_row_types_t lineArray[LF_MAX_ROWS];
	int rowWidth[LF_MAX_ROWS];
	std::vector<std::string> Icon;
	std::vector<bool> marked;


}lf_line_types_struct;

class CListFrame  
{
	private:
		/* Functions */
		void onNewLineArray(void);
		void initVar(void);
		void initFramesRel(void);
		void refreshTitle(void);
		void refreshScroll(void);
		void refreshList(void);
		void refreshHeaderList(void);
		void reSizeMainFrameWidth(int maxTextWidth);
		void reSizeMainFrameHeight(int maxTextHeight);
		int  paintListIcon(int x, int y, int line);
		void paintRowText(const std::string& text, CFont* font, const int& x_pos, const int& y_pos, const int& dx, const int& dy, const fb_pixel_t& col);

		/* Variables */
		lf_line_types_t* m_pLines;

		CBox m_cFrame;
		CBox m_cFrameTitleRel;
		CBox m_cFrameListRel;
		CBox m_cFrameScrollRel;
		CBox m_cFrameHeaderListRel;

		int m_nMaxHeight;
		int m_nMaxWidth;

		int m_nMode;

		int m_nNrOfPages;
		int m_nNrOfLines;
		int m_nNrOfRows;
		int m_nLinesPerPage;
		int m_nCurrentLine;
		int m_nCurrentPage;
		int m_nSelectedLine;

		int m_nBgRadius;
		int* m_cutRowText;

		bool m_showSelection;
		
		CFont* m_pcFontTitle;
		std::string m_textTitle;
		int m_nFontTitleHeight;
		
		CFont* m_pcFontList;
		int m_nFontListHeight;
		
		CFont* m_pcFontHeaderList;
		int m_nFontHeaderListHeight;

		CFrameBuffer * frameBuffer;
	public:
		/* Constructor */
		CListFrame();
		CListFrame(	lf_line_types_t* lines);
		CListFrame(	lf_line_types_t* lines,
					CFont* font_text,
					const int mode, 
					const CBox* position,
					const char* textTitle = NULL,
					CFont* font_title = NULL);

		virtual ~CListFrame();

		/* Functions */
		void    refresh(void);
		void    refreshLine(int line);
		void    scrollPageDown(const int pages);
		void    scrollPageUp(const int pages);				
		void 	scrollLineDown(const int lines);
		void 	scrollLineUp(const int lines);
		bool	setLines(lf_line_types_t* lines);
		static void	addLine2Row(lf_line_types_t* lines, const int& row_num, const std::string& text, CComponentsItem* cc_Item = NULL);
		static void	cleanupRow(lf_line_types_t* lines, const int& row_num);
		bool	setTitle(char* title);
		bool    setSelectedLine(int selection);
		void	setSelectedMarked(bool enable);
		void	setBackGroundRadius(const int radius)
			{
				m_nBgRadius = radius;
				initFramesRel();
			};
		void	clearMarked()
			{
				if (m_pLines)
					for (unsigned i = 0; i < m_pLines->marked.size(); i++)
						m_pLines->marked[i] = false;
			}
		void	hide(void);
		void	paint(void);

inline	CBox	getWindowsPos(void)			{return(m_cFrame);};
inline  int     getSelectedLine(void)		{return(m_nSelectedLine);};
inline  int     getSelectedLineRel(void)	{return(m_nSelectedLine - m_nLinesPerPage*m_nCurrentPage);};
inline  int     getTitleHeight(void)		{return(m_cFrameTitleRel.iHeight);};
inline  int     getHeaderListHeight(void)	{return(m_cFrameHeaderListRel.iHeight);};
inline  int     getLines(void)				{return(m_nNrOfLines);};
inline  int     getPages(void)				{return(m_nNrOfPages);};
inline  void    showSelection(bool show)	{m_showSelection = show;refreshLine(m_nSelectedLine);};
inline	void	movePosition(int x, int y){m_cFrame.iX = x; m_cFrame.iY = y;};
inline	int		getLineHeight()				{return m_nFontListHeight;}
inline	void	cutRowText(int *catRowText)	{m_cutRowText = catRowText;}

		/* Variables */
	typedef enum mode_
	{
		AUTO_WIDTH	= 0x01,
		AUTO_HIGH	= 0x02,
		SCROLL		= 0x04,
		TITLE  		= 0x08,
		HEADER_LINE = 0x80
	}mode;
};

#endif /*LISTFRAME_H_*/
