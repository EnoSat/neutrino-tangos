/*
	Based up Neutrino-GUI - Tuxbox-Project 
	Copyright (C) 2001 by Steffen Hehn 'McClean'

	Classes for generic GUI-related components.
	Copyright (C) 2012-2017, Thilo Graf 'dbt'
	Copyright (C) 2012, Michael Liebmann 'micha-bbg'

	License: GPL

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU General Public
	License along with this program; if not, write to the
	Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
	Boston, MA  02110-1301, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino.h>
#include "cc_item_infobox.h"

using namespace std;

//sub class CComponentsInfoBox from CComponentsItem
CComponentsInfoBox::CComponentsInfoBox(	const int& x_pos,
					const int& y_pos,
					const int& w,
					const int& h,
					std::string info_text,
					const int mode,
					CFont* font_text,
					CComponentsForm *parent,
					int shadow_mode,
					fb_pixel_t color_text,
					fb_pixel_t color_frame,
					fb_pixel_t color_body,
					fb_pixel_t color_shadow)
{
	cc_item_type.id 	= CC_ITEMTYPE_TEXT_INFOBOX;
	cc_item_type.name 	= "cc_info_box";

	x 		= x_old 	= x_pos;
	y 		= y_old 	= y_pos;
	width 		= width_old 	= w;
	height	 	= height_old 	= h;
	shadow		= shadow_mode;
	col_frame 	= color_frame;
	cc_enable_frame	= true;
	col_body_std	= color_body;
	col_shadow	= color_shadow;

	ib_loader	 = NULL;

	ct_text 	= info_text;
	ct_text_mode	= mode;
	ct_font		= font_text;
	ct_col_text	= color_text;

	//CComponentsInfoBox
	pic 		= NULL;
	cctext		= NULL;
	pic_name	= "";
	pic_height	= 0;
	pic_width	= 0;
	x_offset	= OFFSET_INNER_MID;
	initParent(parent);
}

CComponentsInfoBox::~CComponentsInfoBox()
{
	if (ib_loader)
	{
		ib_loader->kill();
		delete ib_loader; ib_loader = NULL;
	}
	delete pic; pic = NULL;
	delete cctext; cctext = NULL;

}

void CComponentsInfoBox::setPicture(const std::string& picture_name, const int& dx, const int& dy)
{
	pic_name = picture_name;
	if (!pic_name.empty())
	{
		frameBuffer->getIconSize(pic_name.c_str(), &pic_width, &pic_height);
		if (dx > -1)
			pic_width = dx;
		if (dy > -1)
			pic_height = dy;
		height = max(pic_height, height);
	}
}

void CComponentsInfoBox::setPicture(const char* picture_name, const int& dx, const int& dy)
{
	string s_tmp = "";
	if (picture_name)
		s_tmp = string(picture_name);
	setPicture(s_tmp, dx, dy);
}

void CComponentsInfoBox::paintPicture()
{
	// NOTE: Real screen values are required, the picon is not used here as usual within a parent form,
	// therefore we imitate exceptionally the layout as if it were embedded in a parent form.
	int x_pic = (cc_parent ? cc_xr : x) + fr_thickness;
	int y_pic = (cc_parent ? cc_yr : y) + fr_thickness;

	// detect possible usage of loader
	bool has_loader = false;
	if (!pic_name.empty())
		has_loader = (pic_name == NEUTRINO_ICON_LOADER);

	// If we have enabled the loader graphic, use this instead picon graphic
	if (has_loader)
	{
		if (!ib_loader)
		{
			ib_loader = new CHourGlass (x_pic+x_offset, y_pic);
			OnBeforeHide.connect(sigc::mem_fun(ib_loader, &CHourGlass::stop));
			OnBeforeKill.connect(sigc::mem_fun(ib_loader, &CHourGlass::stop));
			ib_loader->allowPaint(cc_allow_paint);
		}

		ib_loader->setPos(x_pic+x_offset, y_pic+(height-2*fr_thickness)/2-ib_loader->getHeight()/2);
		ib_loader->paint(true);
	}
	else
	{
		// ensure we have no loader instance
		if (ib_loader)
		{
			delete ib_loader;
			ib_loader = NULL;
		}

		//ensure empty pic object
		if (pic){
			delete pic;
			pic = NULL;
		}

		//exit if no image definied
		if (pic_name.empty())
			return;

		//init pic object and set icon paint position
		pic = new CComponentsPicture(x_pic+x_offset, y_pic, pic_width, min(pic_height, height-2*fr_thickness), pic_name); //NOTE: icons do not scale!

		pic->setColorBody(col_body_std);

		//fit icon into frame
		pic->setYPos(y_pic+(height-2*fr_thickness)/2-pic->getHeight()/2);

		//paint, but set visibility mode
		pic->allowPaint(cc_allow_paint);
		pic->paint(CC_SAVE_SCREEN_NO);
	}
}

void CComponentsInfoBox::paint(const bool &do_save_bg)
{
	paintInit(do_save_bg);
	paintPicture();

	//define text x position
	//NOTE: real values are reqiured, if we paint this item within a form as embedded cc-item
	int x_text = (cc_parent ? cc_xr : x) + fr_thickness;
	int y_text = (cc_parent ? cc_yr : y) + fr_thickness;

	//set text to the left border if picture is not painted
	int pic_w = 0;
	if ((pic) && (pic->isPainted()))
		pic_w = pic->getWidth() + x_offset;
	if (ib_loader)
		pic_w = ib_loader->getWidth() + x_offset;

	//set text properties and paint text lines
 	if (!ct_text.empty()){
 		if (cctext)
			delete cctext;
		cctext = NULL;
	}

	//calculate vars for x-position and dimensions
	int tx = x_offset + x_text + pic_w;
	int tw = width - 2*x_offset - pic_w - 2*fr_thickness;
	int th = height-2*fr_thickness;

	if (cctext == NULL)
		cctext = new CComponentsText(tx, y_text, tw, th);

	cctext->setText(ct_text, ct_text_mode, ct_font);
	cctext->doPaintTextBoxBg(ct_paint_textbg);
	cctext->doPaintBg(false);
	cctext->setTextColor(ct_col_text);
	cctext->enableTboxSaveScreen(cc_txt_save_screen);
	cctext->setDimensionsAll(tx, y_text, tw, th);

	//paint, but set visibility mode
	cctext->allowPaint(cc_allow_paint);
	cctext->paint(CC_SAVE_SCREEN_NO);

	OnAfterPaintInfo();
}
