/*
	Based up Neutrino-GUI - Tuxbox-Project
	Copyright (C) 2001 by Steffen Hehn 'McClean'

	Extra functions based up GUI-related components.
	Copyright (C) 2015, Thilo Graf 'dbt'

	License: GPL

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __CC_EXTRA_H__
#define __CC_EXTRA_H__

#include "cc_item_shapes.h"
#include "cc_item_text.h"

/** 	Paint a box on screen.
*	@param[in] x 				position
*	@param[in] y 				position
*	@param[in] dx 				width
*	@param[in] dy 				height
*	@param[in] color_body			color of background, default = COL_MENUCONTENT_PLUS_0
*	@param[in] radius 			corner radius, default = 0
*	@param[in] corner_type 			corner type, defined in drivers/framebuffer.h
*						@li CORNER_NONE (default)
*						@li CORNER_TOP_LEFT
*						@li CORNER_TOP_RIGHT
*						@li CORNER_TOP
*						@li CORNER_BOTTOM_RIGHT
*						@li CORNER_RIGHT
*						@li CORNER_BOTTOM_LEFT
*						@li CORNER_LEFT
*						@li CORNER_BOTTOM
*						@li CORNER_ALL
*	@param[in] gradient_mode 		mode of color gradient
*						@li CC_COLGRAD_OFF (default)
*						@li CC_COLGRAD_LIGHT_2_DARK
*						@li CC_COLGRAD_DARK_2_LIGHT
*						@li CC_COLGRAD_COL_A_2_COL_B
*						@li CC_COLGRAD_COL_B_2_COL_A
*						@li CC_COLGRAD_COL_LIGHT_DARK_LIGHT
*						@li CC_COLGRAD_COL_DARK_LIGHT_DARK
*	@param[in] gradient_sec_col 		secondary gradient color, default = COL_MENUCONTENT_PLUS_0
*	@param[in] gradient_direction 		mode of color gradient
*						@li CFrameBuffer::gradientVertical (default)
*						@li CFrameBuffer::gradientHorizontal
*	@param[in] gradient_intensity 		gradient intensity
*						@li ColorGradient::light
*						@li ColorGradient::normal (default)
*						@li CFrameBuffer::advanced
*	@param[in] color_frame 			color of frame around box, default = COL_FRAME_PLUS_0
*	@param[in] shadow_mode 			enable/disable shadow behind box, default = CC_SHADOW_OFF
*	@param[in] color_shadow 		color of shadow, default = COL_SHADOW_PLUS_0
*
*	@return
*		True if painted
*	@see
* 		@li CCDraw()
* 		@li CComponentsShapeSquare()
*		@li colors.h
*		@li driver/framebuffer.h
*		@li driver/colorgradient.h
*/
bool paintBoxRel(	const int& x,
			const int& y,
			const int& dx,
			const int& dy,
			const fb_pixel_t& color_body = COL_MENUCONTENT_PLUS_0,
			const int& radius = 0,
			const int& corner_type = CORNER_NONE,
			const int& gradient_mode = CC_COLGRAD_OFF,
			const int& gradient_sec_col = COL_MENUCONTENT_PLUS_0,
			const int& gradient_direction = CFrameBuffer::gradientVertical,
			const int& gradient_intensity = CColorGradient::normal,
			const int& w_frame = 0,
			const fb_pixel_t& color_frame = COL_FRAME_PLUS_0,
			int shadow_mode = CC_SHADOW_OFF,
			const fb_pixel_t& color_shadow = COL_SHADOW_PLUS_0);

/** 	Paint a box on screen.
*	@param[in] x 				position
*	@param[in] y 				position
*	@param[in] dx 				witdh
*	@param[in] dy 				height
*	@param[in] color_body			color of background, default = COL_MENUCONTENT_PLUS_0
*	@param[in] radius 			corner radius, default = 0
*	@param[in] corner_type 			corner type, defined in drivers/framebuffer.h
*						@li CORNER_NONE (default)
*						@li CORNER_TOP_LEFT
*						@li CORNER_TOP_RIGHT
*						@li CORNER_TOP
*						@li CORNER_BOTTOM_RIGHT
*						@li CORNER_RIGHT
*						@li CORNER_BOTTOM_LEFT
*						@li CORNER_LEFT
*						@li CORNER_BOTTOM
*						@li CORNER_ALL
*	@param[in] color_frame 			color of frame around box, default = COL_FRAME_PLUS_0
*	@param[in] shadow_mode 			enable/disable shadow behind box, default = CC_SHADOW_OFF
*	@param[in] color_shadow 		color of shadow, default = COL_SHADOW_PLUS_0
*
*	@return
*		True if painted
*	@see
* 		@li CCDraw()
* 		@li CComponentsShapeSquare()
*		@li colors.h
*		@li driver/framebuffer.h
*		@li driver/colorgradient.h
*/
bool paintBoxRel0(	const int& x,
			const int& y,
			const int& dx,
			const int& dy,
			const fb_pixel_t& color_body,
			const int& radius = 0,
			const int& corner_type = CORNER_NONE,
			const int& w_frame = 0,
			const fb_pixel_t& color_frame = COL_FRAME_PLUS_0,
			int shadow_mode = CC_SHADOW_OFF,
			const fb_pixel_t& color_shadow = COL_SHADOW_PLUS_0);

/** 	Paint a text box on screen.
*	@param[in] std::string& 		text
*	@param[in] x 				position
*	@param[in] y 				position
*	@param[in] dx 				width
*	@param[in] dy 				height
*	@param[in] *font			pointer to font type object, default = NULL, sets g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO] as default font
*	@param[in] color_body			color of box, default = COL_MENUCONTENT_PLUS_0
*	@param[in] font_style			font style
*						@li CComponentsText::FONT_STYLE_REGULAR (default)
*						@li CComponentsText::FONT_STYLE_BOLD,
*						@li CComponentsText::FONT_STYLE_ITALIC
*	@param[in] radius 			corner radius, default = 0
*	@param[in] corner_type 			corner type, defined in drivers/framebuffer.h
*						@li CORNER_NONE (default)
*						@li CORNER_TOP_LEFT
*						@li CORNER_TOP_RIGHT
*						@li CORNER_TOP
*						@li CORNER_BOTTOM_RIGHT
*						@li CORNER_RIGHT
*						@li CORNER_BOTTOM_LEFT
*						@li CORNER_LEFT
*						@li CORNER_BOTTOM
*						@li CORNER_ALL
*	@param[in] gradient_mode 		mode of color gradient
*						@li CC_COLGRAD_OFF (default)
*						@li CC_COLGRAD_LIGHT_2_DARK
*						@li CC_COLGRAD_DARK_2_LIGHT
*						@li CC_COLGRAD_COL_A_2_COL_B
*						@li CC_COLGRAD_COL_B_2_COL_A
*						@li CC_COLGRAD_COL_LIGHT_DARK_LIGHT
*						@li CC_COLGRAD_COL_DARK_LIGHT_DARK
*	@param[in] gradient_sec_col 		secondary gradient color, default = COL_MENUCONTENT_PLUS_0
*	@param[in] gradient_direction 		mode of color gradient
*						@li CFrameBuffer::gradientVertical (default)
*						@li CFrameBuffer::gradientHorizontal
*	@param[in] gradient_intensity 		gradient intensity
*						@li ColorGradient::light
*						@li ColorGradient::normal (default)
*						@li CFrameBuffer::advanced
*	@param[in] color_frame 			color of frame around box, default = COL_FRAME_PLUS_0
*	@param[in] shadow_mode 			enable/disable shadow behind box, default = CC_SHADOW_OFF
*	@param[in] color_shadow 		color of shadow, default = COL_SHADOW_PLUS_0
*
*	@return
*		True if painted
*	@see
*		@li CCDraw()
*		@li CComponentsText()
*		@li CComponentsLabel()
*		@li CTextBox()
*		@li colors.h
*		@li driver/framebuffer.h
*		@li driver/colorgradient.h
*/
bool paintTextBoxRel(	const std::string& text,
			const int& x,
			const int& y,
			const int& dx,
			const int& dy = 0,
			CFont *font = NULL,
			const int& mode = CTextBox::AUTO_WIDTH,
			const int& font_style = CComponentsText::FONT_STYLE_REGULAR,
			const fb_pixel_t& color_text = COL_MENUCONTENT_TEXT,
			const fb_pixel_t& color_body = COL_MENUCONTENT_PLUS_0,
			const int& radius = 0,
			const int& corner_type = CORNER_NONE,
			const int& gradient_mode = CC_COLGRAD_OFF,
			const int& gradient_sec_col = COL_MENUCONTENT_PLUS_0,
			const int& gradient_direction = CFrameBuffer::gradientVertical,
			const int& gradient_intensity = CColorGradient::normal,
			const fb_pixel_t& color_frame = COL_FRAME_PLUS_0,
			int shadow_mode = CC_SHADOW_OFF,
			const fb_pixel_t& color_shadow = COL_SHADOW_PLUS_0);

/** 	Paints an image on screen with defined position. If no dimensions are defined, this method will try to paint an image as icon without any scale.\n
*	If an dimension is defined it will be try to scale the image.\n
*	Default behavior is paint an icon on screen like known method paintIcon() in class CFrameBuffer.
*	@param[in] std::string& 		full path to image file or icon filename without file extension (e.g. .png)
*	@param[in] x 				position
*	@param[in] y 				position
*	@param[in] dx 				width, default = 0 (no scale)
*	@param[in] dy 				height, default = 0 (no scale)
*	@param[in] transparent			image transparency mode
*						@li CFrameBuffer::TM_EMPTY
						@li CFrameBuffer::TM_NONE (default)
						@li CFrameBuffer::TM_BLACK
						@li CFrameBuffer::TM_INI
*	@param[in] color_body			color of background, default = 0 (no background)
*	@param[in] radius 			corner radius of background, default = 0
*	@param[in] corner_type 			corner type of background, defined in drivers/framebuffer.h
*						@li CORNER_NONE (default)
*						@li CORNER_TOP_LEFT
*						@li CORNER_TOP_RIGHT
*						@li CORNER_TOP
*						@li CORNER_BOTTOM_RIGHT
*						@li CORNER_RIGHT
*						@li CORNER_BOTTOM_LEFT
*						@li CORNER_LEFT
*						@li CORNER_BOTTOM
*						@li CORNER_ALL
*	@param[in] gradient_mode 		mode of background color gradient
*						@li CC_COLGRAD_OFF (default)
*						@li CC_COLGRAD_LIGHT_2_DARK
*						@li CC_COLGRAD_DARK_2_LIGHT
*						@li CC_COLGRAD_COL_A_2_COL_B
*						@li CC_COLGRAD_COL_B_2_COL_A
*						@li CC_COLGRAD_COL_LIGHT_DARK_LIGHT
*						@li CC_COLGRAD_COL_DARK_LIGHT_DARK
*	@param[in] gradient_sec_col 		secondary gradient background color, default = COL_MENUCONTENT_PLUS_0
*	@param[in] gradient_direction 		background color gradient direction
*						@li CFrameBuffer::gradientVertical (default)
*						@li CFrameBuffer::gradientHorizontal
*	@param[in] gradient_intensity 		background gradient intensity
*						@li ColorGradient::light
*						@li ColorGradient::normal (default)
*						@li CFrameBuffer::advanced
*	@param[in] color_frame 			color of frame around box, default = COL_FRAME_PLUS_0
*	@param[in] shadow_mode 			enable/disable shadow behind box, default = CC_SHADOW_OFF
*	@param[in] color_shadow 		color of shadow, default = COL_SHADOW_PLUS_0
*
*	@return
*		True if painted
*	@see
*		@li icons.h
*		@li colors.h
*		@li CComponentsPicture()
*		@li driver/framebuffer.h
*		@li driver/colorgradient.h
*/
bool paintImage(	const std::string& Image,
			const int& x,
			const int& y,
			const int& dx = 0,
			const int& dy = 0,
			const int& transparent = CFrameBuffer::TM_EMPTY,
			const fb_pixel_t& color_body = 0,
			const int& radius = 0,
			const int& corner_type = CORNER_NONE,
			const fb_pixel_t& color_frame = COL_FRAME_PLUS_0,
			int shadow_mode = CC_SHADOW_OFF,
			const fb_pixel_t& color_shadow = COL_SHADOW_PLUS_0);

/**	Paints a box with icon and optional text as content on screen with defined position and dimensions.\n
 *	If no dimensions are defined, this method will try to paint the box without any scale.\n
 *	If an dimension is defined it will be try to scale the image.\n
 *	Default behavior is paint an icon on screen like known method paintIcon() in class CFrameBuffer.
 *	@param[in] std::string& 		icon filename without file extension (e.g. .png)
 *	@param[in] x 				position
 *	@param[in] y 				position
 *	@param[in] dx 				width, default = 0 (no scale)
 *	@param[in] dy 				height, default = 0 (no scale)
 *	@param[in] std::string& 		text (default empty)
 *	@param[in] font_style			font style
 *						@li CComponentsText::FONT_STYLE_REGULAR (default)
 *						@li CComponentsText::FONT_STYLE_BOLD,
 *						@li CComponentsText::FONT_STYLE_ITALIC
 *	@param[in] color_body			color of background, default = COL_MENUCONTENT_PLUS_0
 *	@param[in] color_text 			color of text, default = COL_MENUCONTENT_TEXT
 *
 *	@return
 *		True if painted
 *	@see
 *		@li paintImage()
 */
bool paintIcon (	const std::string& filename,
			const int& x,
			const int& y,
			const int& dx,
			const int& dy,
			const std::string& text = std::string(),
			const int& font_style = CComponentsText::FONT_STYLE_REGULAR,
			const fb_pixel_t& color_body = COL_MENUCONTENT_PLUS_0,
			const fb_pixel_t& color_text = COL_MENUCONTENT_TEXT);

/**	Overloaded version of paintIcon() without dimension parameters Paints a box with icon and optional text as content on screen with defined position.\n
 *	If no dimensions are defined, this method will try to paint the box without any scale.\n
 *	If an dimension is defined it will be try to scale the image.\n
 *	Default behavior is paint an icon on screen like known method paintIcon() in class CFrameBuffer.
 *	@param[in] std::string& 		icon filename without file extension (e.g. .png)
 *	@param[in] x 				position
 *	@param[in] y 				position
 *	@param[in] std::string& 		text (default empty)
 *	@param[in] font_style			font style
 *						@li CComponentsText::FONT_STYLE_REGULAR (default)
 *						@li CComponentsText::FONT_STYLE_BOLD,
 *						@li CComponentsText::FONT_STYLE_ITALIC
 *	@param[in] color_body			color of background, default = COL_MENUCONTENT_PLUS_0
 *	@param[in] color_text 			color of text, default = COL_MENUCONTENT_TEXT
 *
 *	@return
 *		True if painted
 *	@see
 *		@li paintImage()
 *		@li paintIcon()
 */
bool paintIcon (	const std::string& filename,
			const int& x,
			const int& y,
			const std::string& text = std::string(),
			const int& font_style = CComponentsText::FONT_STYLE_REGULAR,
			const fb_pixel_t& color_body = COL_MENUCONTENT_PLUS_0,
			const fb_pixel_t& color_text = COL_MENUCONTENT_TEXT);

#endif
