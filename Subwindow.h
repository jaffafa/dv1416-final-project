#ifndef SUBWINDOW_H
#define SUBWINDOW_H

#include "StdAfx.h"
#include "EventReceiver.h"

namespace GUI
{
	struct SubwindowDesc
	{
		std::string caption;
		UINT		x;
		UINT		y;
	};

	class Subwindow
	{
	public:
		Subwindow(void);
		virtual ~Subwindow(void);

		virtual void init(HINSTANCE hInstance, HWND hParentWnd,
						  const SubwindowDesc subwindowDesc) = 0;
		virtual LRESULT subWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		void show(const bool show);
	protected:
		HINSTANCE m_hInstance;
		HWND	  m_hWnd;

		SubwindowDesc m_subwindowDesc;
		DWORD		  m_style;
		UINT		  m_clientWidth;
		UINT		  m_clientHeight;

		std::vector<EventElement> m_items;

		void initWindow(HINSTANCE hInstance, HWND hParentWnd, const SubwindowDesc subwindowDesc,
						const DWORD style, const UINT clientWidth, const UINT clientHeight);
	
		POINT getWindowSize(void) const;
		POINT getWindowPosition(void) const;

		virtual int getItemID(const UINT i) const = 0;
	};
}

#endif