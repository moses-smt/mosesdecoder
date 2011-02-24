/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2010 University of Edinburgh

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#include "Span.h"

bool
spansIntersect(const Span & a, const Span & b)
{
  for (Span::const_iterator p(a.begin()); p != a.end(); ++p) {
    Span::const_iterator q = b.find(*p);
    if (q != b.end()) {
      return true;
    }
  }
  return false;
}

Span
closure(const Span & s)
{
  Span result;
  if (s.empty()) {
    return result;
  }
  Span::const_iterator p(s.begin());
  int min = *p;
  int max = *p;
  ++p;
  for (; p != s.end(); ++p) {
    if (*p < min) {
      min = *p;
    }
    if (*p > max) {
      max = *p;
    }
  }
  for (int i = min; i <= max; ++i) {
    result.insert(i);
  }

  return result;
}
