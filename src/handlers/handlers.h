/*
    This file is part of inputmangler, a programm which intercepts and
    transforms linux input events, depending on the active window.
    Copyright (C) 2014  Arkadiusz Guzinski <kermit@ag.de1.cc>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "devhandler.h"
#include "nethandler.h"
#include "debughandler.h"
#include "xwatcher.h"

/*!
 * @brief  all handlers have to register their parsing functions here.
 */
void registerHandlers()
{
	AbstractInputHandler::registerParser("device",   DevHandler::parseXml);
	AbstractInputHandler::registerParser("debug",    DebugHandler::parseXml);
	AbstractInputHandler::registerParser("net",      NetHandler::parseXml);
	AbstractInputHandler::registerParser("xwatcher", XWatcher::parseXml);
}