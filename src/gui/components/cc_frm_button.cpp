/*
	Based up Neutrino-GUI - Tuxbox-Project 
	Copyright (C) 2001 by Steffen Hehn 'McClean'

	Classes for generic GUI-related components.
	Copyright (C) 2012-2014, Thilo Graf 'dbt'

	License: GPL

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

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
#include <gui/color_custom.h>
#include <system/debug.h>
#include "cc_frm_button.h"
#include <driver/fontrenderer.h>

using namespace std;

CComponentsButton::CComponentsButton( 	const int& x_pos, const int& y_pos, const int& w, const int& h,
					const std::string& caption, const std::string& icon_name,
					CComponentsForm* parent,
					bool selected,
					bool enabled,
					int shadow_mode,
					fb_pixel_t color_frame, fb_pixel_t color_body, fb_pixel_t color_shadow)
{
	cc_btn_text_locale = NONEXISTANT_LOCALE;
	initVarButton(x_pos, y_pos, w, h,  caption, icon_name, parent, selected, enabled, shadow_mode, color_frame, color_body, color_shadow);
}

CComponentsButton::CComponentsButton( 	const int& x_pos, const int& y_pos, const int& w, const int& h,
					const neutrino_locale_t& caption_locale, const std::string& icon_name,
					CComponentsForm* parent,
					bool selected,
					bool enabled,
					int shadow_mode,
					fb_pixel_t color_frame, fb_pixel_t color_body, fb_pixel_t color_shadow)
{
	cc_btn_text_locale = caption_locale;
	initVarButton(x_pos, y_pos, w, h, g_Locale->getText(cc_btn_text_locale), icon_name, parent, selected, enabled, shadow_mode, color_frame, color_body, color_shadow);
}

CComponentsButton::CComponentsButton( 	const int& x_pos, const int& y_pos, const int& w, const int& h,
					const std::string& caption, const char* icon_name,
					CComponentsForm* parent,
					bool selected,
					bool enabled,
					int shadow_mode,
					fb_pixel_t color_frame, fb_pixel_t color_body, fb_pixel_t color_shadow)
{
	string _icon_name = icon_name == NULL ? "" : string(icon_name);
	initVarButton(x_pos, y_pos, w, h,  caption, _icon_name, parent, selected, enabled, shadow_mode, color_frame, color_body, color_shadow);
}

CComponentsButton::CComponentsButton( 	const int& x_pos, const int& y_pos, const int& w, const int& h,
					const neutrino_locale_t& caption_locale, const char* icon_name,
					CComponentsForm* parent,
					bool selected,
					bool enabled,
					int shadow_mode,
					fb_pixel_t color_frame, fb_pixel_t color_body, fb_pixel_t color_shadow)
{
	string _icon_name = icon_name == NULL ? "" : string(icon_name);
	cc_btn_text_locale = caption_locale;
	initVarButton(x_pos, y_pos, w, h,  g_Locale->getText(cc_btn_text_locale), _icon_name, parent, selected, enabled, shadow_mode, color_frame, color_body, color_shadow);
}

void CComponentsButton::initVarButton(	const int& x_pos, const int& y_pos, const int& w, const int& h,
					const std::string& caption,
					const std::string& icon_name,
					CComponentsForm* parent,
					bool selected,
					bool enabled,
					int shadow_mode,
					fb_pixel_t color_frame, fb_pixel_t color_body, fb_pixel_t color_shadow)
{
	cc_item_type.id = CC_ITEMTYPE_BUTTON;
	cc_item_type.name = caption.empty() ? "cc_button" :  caption;

	x = cc_xr = cc_xr_old = x_old	= x_pos;
	y = cc_yr = cc_yr_old = y_old	= y_pos;
	width 		= max(w, 0);
	height	 	= max(h, 0);
	shadow		= shadow_mode;
	shadow_w	= shadow != CC_SHADOW_OFF ? (shadow_w == -1 ? OFFSET_SHADOW/2 : shadow_w) : 0; //buttons are mostly small elements, so these elements should have a reasonable shadow width

	cc_body_gradient_enable = CC_COLGRAD_OFF/*g_settings.gradiant*/; //TODO: gradient is prepared for use but disabled at the moment till some other parts of gui parts are provide gradient
	setColBodyGradient(cc_body_gradient_enable/*CColorGradient::gradientLight2Dark*/, CFrameBuffer::gradientVertical, CColorGradient::light);
	col_frame 	= color_frame;
	col_body_std	= cc_body_gradient_enable? COL_DARK_GRAY : color_body;
	col_shadow	= color_shadow;

	cc_item_enabled  = enabled;
	cc_item_selected = selected;
	fr_thickness 	= 0; //TODO: parts of the GUI still don't use framed buttons
	append_x_offset = OFFSET_INTER;
	append_y_offset = 0;
	corner_rad	= RADIUS_SMALL;
	
	cc_btn_text_col = cc_btn_text_std_col = cc_btn_text_sel_col = cc_body_gradient_enable ? COL_BUTTON_TEXT_ENABLED : COL_MENUFOOT_TEXT;
	cc_btn_text_disable_col = cc_body_gradient_enable ? COL_BUTTON_TEXT_DISABLED : COL_MENUCONTENTINACTIVE_TEXT;
	cc_btn_icon_obj	= NULL;
	cc_btn_text_obj = NULL;
	cc_btn_dy_font  = CNeutrinoFonts::getInstance();
	cc_btn_font	= NULL;
	cc_btn_icon	= icon_name;
	cc_btn_text	= caption;
	cc_directKeys.push_back(CRCInput::RC_nokey);
	cc_btn_result	= -1;
	cc_btn_alias	= -1;

	initCCBtnItems();
	initParent(parent);
}

void CComponentsButton::initIcon()
{
	//init cch_icon_obj only if an icon available
	if (cc_btn_icon.empty())
	{
		if (cc_btn_icon_obj)
			delete cc_btn_icon_obj;
		cc_btn_icon_obj = NULL;
		return;
	}

	//init icon object
	if (cc_btn_icon_obj == NULL)
		cc_btn_icon_obj = new CComponentsPicture(fr_thickness, 0, cc_btn_icon, this);

	//get required icon height and position
	int dx_tmp, dy_tmp = 0;
	cc_btn_icon_obj->getRealSize(&dx_tmp, &dy_tmp);

	int h_icon = height-2*fr_thickness - 2*OFFSET_INNER_MIN;
	h_icon = min(h_icon, dy_tmp);

	int y_icon = height/2 - h_icon/2;

	cc_btn_icon_obj->setHeight(h_icon);
	cc_btn_icon_obj->setYPos(y_icon);
}

void CComponentsButton::initCaption()
{
	//init label as caption object and add to container
	if (!cc_btn_text.empty()){
		if (cc_btn_text_obj == NULL){
			cc_btn_text_obj = new CComponentsLabel();
			cc_btn_text_obj->doPaintBg(false);
			cc_btn_text_obj->doPaintTextBoxBg(false);
			cc_btn_text_obj->enableTboxSaveScreen(cc_txt_save_screen);
			addCCItem(cc_btn_text_obj);
		}
	}else{
		if (cc_btn_text_obj){
			delete cc_btn_text_obj;
			cc_btn_text_obj = NULL;
		}
	}

	//set basic properties
	int x_offset = (!cc_btn_text_obj || !cc_btn_icon_obj) ? 0 : append_x_offset;
	int w_frame = fr_thickness;
	int frame = 2*w_frame;
	if (cc_btn_text_obj){
		//position and size
		int x_cap = w_frame;
		int dx_icon_obj = cc_btn_icon_obj ? cc_btn_icon_obj->getWidth() : 0;
		x_cap += dx_icon_obj;

		/* use system defined font as default if not defined */
		if (cc_btn_font == NULL)
			cc_btn_font = g_Font[SNeutrinoSettings::FONT_TYPE_BUTTON_TEXT];

		//text (caption) and font
		/* If button dimension too small, use dynamic font, this ignores possible defined font
		 * Otherwise definied font will be used. Button dimensions could be defined directly or calculated from a parent container (e.g. footer...).
		 * These dimensions must be enough to display complete content like possible icon and without truncated text.
		 */
		CFont *tmp_font = cc_btn_font;
		int h_cap = min(cc_btn_text_obj->getHeight(), tmp_font->getHeight()) + OFFSET_INNER_MIN;
		int w_cap = max(tmp_font->getRenderWidth(cc_btn_text),  width - x_offset - dx_icon_obj - 2*frame);
		int dx_tmp = x_offset + dx_icon_obj + w_cap - frame;

		/* If the required sum for space of icon, text, frame and offsets is too small then adapt font size.
		 */
		if (dx_tmp > width){
			if (width == 0){
				width = dx_tmp;
			}else{
				int diff = dx_tmp - width;
				width -= diff;
				w_cap -= diff;
			}
			tmp_font = *cc_btn_dy_font->getDynFont(w_cap, h_cap, cc_btn_text);
			cc_btn_font = tmp_font;
// 			dprintf(DEBUG_NORMAL, "[CComponentsButton]\t[%s - %d]\t%s dx_tmp [%d], width [%d] diff [%d] w_cap [%d] h_cap [%d]\n", __func__, __LINE__, cc_btn_text.c_str(), dx_tmp, width, diff, w_cap, h_cap);
		}

		/*NOTE:
			paint of centered text in y direction without y_offset
			looks unlovely displaced in y direction especially besides small icons and inside small areas,
			but text render isn't wrong here, because capitalized chars or long chars like e. 'q', 'y' are considered!
			Therefore we here need other icons or a hack, that considers some different height values.
		*/
		int y_cap = height/2 - h_cap/2;

		cc_btn_text_obj->setDimensionsAll(x_cap, y_cap, w_cap, h_cap);

		cc_btn_text_obj->setText(cc_btn_text, CTextBox::NO_AUTO_LINEBREAK, cc_btn_font);
		cc_btn_text_obj->forceTextPaint(); //here required;
		cc_btn_text_obj->getCTextBoxObject()->setTextBorderWidth(0,0);

		//set color
		if (isEnabled())
		{
			if(isSelected())
				cc_btn_text_col = cc_btn_text_sel_col;
			else
				cc_btn_text_col = cc_btn_text_std_col;
		}
		else
			cc_btn_text_col = cc_btn_text_disable_col;

		cc_btn_text_obj->setTextColor(cc_btn_text_col);

		//corner of text item
		cc_btn_text_obj->setCorner(corner_rad-w_frame, corner_type);
	}

	//handle common position of icon and text inside container required for alignment
	int w_required 	= w_frame + x_offset;
	w_required 	+= cc_btn_icon_obj ? cc_btn_icon_obj->getWidth() + x_offset : 0;
	w_required 	+= cc_btn_font ? cc_btn_font->getRenderWidth(cc_btn_text) : 0;
	w_required 	+= x_offset + w_frame;

	//dynamic width
	if (w_required > width){
		dprintf(DEBUG_INFO, "[CComponentsButton]   [%s - %d] width of button (%s) will be changed: defined width=%d, required width=%d\n", __func__, __LINE__, cc_btn_text.c_str(), width, w_required);
		width = max(w_required, width);
	}

	//do center
	int x_icon = width/2 - w_required/2 /*+ fr_thickness + x_offset*/;
	int w_icon = 0;
	if (cc_btn_icon_obj){
		x_icon += w_frame + x_offset;
		cc_btn_icon_obj->setXPos(x_icon);
		w_icon = cc_btn_icon_obj->getWidth();
		/*in case of dynamic changed height of caption or button opbject itself,
		 *we must ensure centered y position of icon object
		*/
		int y_icon = height/2 - cc_btn_icon_obj->getHeight()/2;
		cc_btn_icon_obj->setYPos(y_icon);
	}
	if (cc_btn_text_obj){
		cc_btn_text_obj->setXPos(x_icon + w_icon + x_offset);
		cc_btn_text_obj->setWidth(width - cc_btn_text_obj->getXPos());
	}
}

void CComponentsButton::setCaption(const std::string& text)
{
	cc_btn_text = text;
	initCCBtnItems();
}

void CComponentsButton::setCaption(const neutrino_locale_t locale_text)
{
	cc_btn_text_locale = locale_text;
	setCaption(g_Locale->getText(cc_btn_text_locale));
}

void CComponentsButton::setButtonIcon(const std::string& icon)
{
	cc_btn_icon = icon;
	initCCBtnItems();
}

void CComponentsButton::initCCBtnItems()
{
	initIcon();

	initCaption();
}


void CComponentsButton::paint(const bool &do_save_bg)
{
	//prepare items before paint
	initCCBtnItems();

	//paint form contents
	paintForm(do_save_bg);
}
