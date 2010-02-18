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
	\version \$Id: mkxdsProperty.cpp,v 1.3 2003/07/23 14:17:18 toff Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/

#include <streams.h>
#include <commctrl.h>
#include <qnetwork.h> // for IAMMediaContent

#include "mkxdsProperty.h"
#include "resource.h"

MkxFilterProperty::MkxFilterProperty(LPUNKNOWN lpunk, HRESULT *phr)
	:CBasePropertyPage(NAME("Matroska Filter Property Page"), lpunk,
	                   IDD_DIALOG_PROPERTY_MAIN, IDS_STRING_PROP_TITLE)
	,m_pFilter(NULL)
{
    InitCommonControls();
}

CUnknown * WINAPI MkxFilterProperty::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);

    CUnknown *punk = new MkxFilterProperty(lpunk, phr);

    if(punk == NULL)
    {
        if (phr)
            *phr = E_OUTOFMEMORY;
    }

    return punk;
}

HRESULT MkxFilterProperty::OnConnect(IUnknown *pUnk)
{
    if (pUnk == NULL)
    {
        return E_POINTER;
    }
    ASSERT(m_pFilter == NULL);
    return pUnk->QueryInterface(IID_IMkxFilter, reinterpret_cast<void**>(&m_pFilter));
}

HRESULT MkxFilterProperty::OnDisconnect()
{
    // Release of Interface after setting the appropriate contrast value
    if (!m_pFilter)
        return E_UNEXPECTED;

    m_pFilter->Release();
    m_pFilter = NULL;

    return NOERROR;

}

BOOL MkxFilterProperty::OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}

// ============================================================================