/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * MIT License:
 *
 * Copyright (c) 2009 Alexei Svitkine, Eugene Sandulenko
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "common/list.h"
#include "common/array.h"

#include "graphics/surface.h"

#include "wage/wage.h"
#include "wage/macwindow.h"
#include "wage/macwindowmanager.h"

namespace Wage {

MacWindowManager::MacWindowManager() {
    _lastId = 0;
    _activeWindow = -1;
}

MacWindowManager::~MacWindowManager() {
    for (uint i = 0; i < _lastId; i++)
        delete _windows[i];
}

int MacWindowManager::add(bool scrollable) {
    MacWindow *w = new MacWindow(scrollable);

    _windows.push_back(w);
    _windowStack.push_back(w);

    _activeWindow = _lastId;

    _lastId++;

    return _activeWindow;
}

void MacWindowManager::setActive(int id) {
    if (_activeWindow == id)
        return;

    if (_activeWindow != -1)
        _windows[_activeWindow]->setActive(false);

    _windows[id]->setActive(true);

    _windowStack.remove(_windows[id]);
    _windowStack.push_back(_windows[id]);

    _fullRefresh = true;
}

void MacWindowManager::draw(Graphics::Surface *g) {
    for (Common::List<MacWindow *>::const_iterator it = _windowStack.begin(); it != _windowStack.end(); it++)
        (*it)->draw(g, _fullRefresh);

    _fullRefresh = false;
}

} // End of namespace Wage
