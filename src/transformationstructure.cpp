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

#include "transformationstructure.h"
#include <QRegularExpression>
#include <QDebug>

/*!
 * @brief get the window structure for window w
 * @param w window class
 * @param create if true: create a new window, when none is found (may be obsolete since rewrite of the parsing code)
 */
WindowSettings* TransformationStructure::window(QString w, bool create)
{
	if (create)
	{
		if(!classes.contains(w))
			classes.insert(w, new WindowSettings());
		return classes.value(w);
	}
	if (!classes.contains(w))
		return NULL;
	return classes.value(w);
}

/*!
 * @brief add WindowSettings w for window class s
 * @param s
 * @param w 
 */
void TransformationStructure::addWindowSettings(QString s, WindowSettings* w)
{
	classes.insert(s, w);
}

/*!
 * @brief get output events for a given window and window title
 */
QVector< OutEvent* > TransformationStructure::getOutputs(QString window_class, QString window_name)
{
	//qDebug() << "getOutputs(" << c << ", " << n << ")";
	WindowSettings *w = window(window_class);
	if (w == NULL)
		return def;
	//qDebug() << "Window found with " << w->titles.size() << "titles";
	for (int i = 0; i < w->titles.size(); i++)
	{
		//qDebug() << "Title: " << w->titles.at(i)->pattern();
		QRegularExpressionMatch m = w->titles.at(i)->match(window_name);
		if(m.hasMatch())
			return w->events.at(i);
	}
	return w->def;
}

/*!
 * @brief deletes all titles
 */
WindowSettings::~WindowSettings()
{
	foreach (QRegularExpression* r, titles)
		delete r;
}

/*!
 * @brief deletes all window classes
 */
TransformationStructure::~TransformationStructure()
{
	foreach (WindowSettings * w, classes)
		delete w;
}

/*!
 * @brief Check a TransformationStructure for configuration errors.
 * @param numInputs Number of expected inputs.
 * @param id Id of the TransformationStructure.
 * @param verbose If true, the complete TransformationStructure will be printed to console.
 * @return True if TransformationStructure seems ok.
 */
bool TransformationStructure::sanityCheck(int numInputs, QString id, bool verbose)
{
	bool result = true;
	qDebug() << "\nchecking " << id << " with size " << numInputs;

	if (verbose) {
		QString tmp = "Defaults: ";
		for (int j = 0; j < def.size(); j++) {
			tmp += def.at(j)->toString();
			if (j < def.size() - 1)
				tmp + ", ";
		}
        qDebug() << tmp;
    }

	if (this->def.size() != numInputs)
	{
		qDebug() << "TransformationStructure.def is " << this->def.size();
		result = false;
	}
	QList<WindowSettings*> wlist = classes.values();
	foreach (WindowSettings *w, wlist)
	{
		if (verbose)
		{
			qDebug() << "Settings for Window = " << classes.key(w);
			QString s = "  ";
			for (int j = 0; j < w->def.size(); j++)
			{
				s += w->def.at(j)->toString();
				if (j < w->def.size() - 1)
					s += ", ";
			}
			qDebug() << s;
		}
		if (w->def.size() != numInputs)
		{
			qDebug() << "WindowSettings.def for " << classes.key(w) << " is " << w->def.size();
			result = false;
		}
		if (w->events.size() != w->titles.size())
		{
			qDebug() << "WindowSettings.titles = " << w->titles.size() 
					 << " events = " << w->events.size() << " for " << classes.key(w);
			result = false;
		}
		for(int i = 0; i < w->events.size(); i++)
		{
            try {
				if (verbose) {
					qDebug() << "  with Pattern: \"" << w->titles[i]->pattern() << "\"";
					QString s = "    ";
					for (int j = 0; j < w->events.at(i).size(); j++) {
						s += w->events.at(i).at(j)->toString();
						if (j < w->events.at(i).size() - 1)
							s += ", ";
					}
					qDebug() << s;
				}
				if (w->events.at(i).size() != numInputs) {
					qDebug() << "WindowSettings for " << classes.key(w) << ":"
							 << "Regex = \"" << w->titles.at(i)->pattern() << "\", size = "
							 << w->events.at(i).size();
					result = false;
				}
			} catch (std::bad_alloc& ba) {
				qDebug() << ba.what();
			} catch (void *) {
				qDebug() << "something else went terribly wrong while checking Output events in a <title>";
			}
		}
	}
	if (!result)
		qDebug() << id << " failed SanityCheck!!!";
	return result;
}
