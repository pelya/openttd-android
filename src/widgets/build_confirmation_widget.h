/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file build_confirmation_widget.h Types related to the build_confirmation widgets. */

#ifndef WIDGETS_BUILD_CONFIRMATION_WIDGET_H
#define WIDGETS_BUILD_CONFIRMATION_WIDGET_H

/** Widgets of the #BuildVehicleWindow class. */
enum BuildConfirmationWidgets {
	WID_BC_PRICE,                     ///< Estimated price.
	WID_BC_OK,                        ///< Confirm action.
	WID_BC_CANCEL,                    ///< Cancel action.
	WID_BC_PANEL,                     ///< Cancel action.
};

#endif /* WIDGETS_BUILD_CONFIRMATION_WIDGET_H */
