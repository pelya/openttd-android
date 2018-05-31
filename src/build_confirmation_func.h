/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file build_confirmation_func.h Transparent confirmation dialog for building anything on the map. */

#ifndef BUILD_CONFIRMATION_FUNC_H
#define BUILD_CONFIRMATION_FUNC_H

#include "stdafx.h"
#include "window_func.h"
#include "widget_type.h"


void ShowBuildConfirmationWindow();
void HideBuildConfirmationWindow();
bool ConfirmationWindowShown();
bool BuildConfirmationWindowProcessViewportClick();
bool ConfirmationWindowEstimatingCost();
void ConfirmationWindowSetEstimatedCost(Money cost);

#endif /* BUILD_CONFIRMATION_FUNC_H */
