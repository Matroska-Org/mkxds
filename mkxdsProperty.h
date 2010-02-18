/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2003 Steve Lhomme.  All rights reserved.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding an other license may use this file in accordance with 
** the Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.matroska.org/license/qpl/ for QPL licensing information.
** See http://www.matroska.org/license/gpl/ for GPL licensing information.
**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/
/*!
	\file
	\version \$Id: mkxdsProperty.h,v 1.2 2003/07/20 18:22:13 jcsston Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/

#ifndef MKXDS_FILTER_PROPERTY_H
#define MKXDS_FILTER_PROPERTY_H

#include "mkxds.h"

// {54A3F9E6-4913-4ae4-98F7-2B0E19D8C848}
DEFINE_GUID(CLSID_MKXFILTERPROP, 
0x54a3f9e6, 0x4913, 0x4ae4, 0x98, 0xf7, 0x2b, 0xe, 0x19, 0xd8, 0xc8, 0x48);

class MkxFilterProperty : public CBasePropertyPage 
{
public:

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

protected:
    MkxFilterProperty(LPUNKNOWN lpunk, HRESULT *phr);

	BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnDisconnect();

    IMkxFilter *m_pFilter;

    IMkxFilter *pFilter() {
        ASSERT(m_pFilter);
        return m_pFilter;
    };
};

#endif // MKXDS_FILTER_PROPERTY_H
