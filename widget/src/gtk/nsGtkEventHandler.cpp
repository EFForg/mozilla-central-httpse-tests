/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "gtk/gtk.h"
#include "nsGtkEventHandler.h"

#include "nsWindow.h"
#include "nsTextWidget.h"
#include "nsCheckButton.h"
#include "nsRadioButton.h"
#include "nsFileWidget.h"
#include "nsGUIEvent.h"
#include "nsIMenuItem.h"

#include "stdio.h"

#define DBG 0

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

struct nsKeyConverter {
  int vkCode; // Platform independent key code
  XID keysym; // X keysym key code
};

struct nsKeyConverter nsKeycodes[] = { 
  NS_VK_CANCEL,     XK_Cancel, 
  NS_VK_BACK,       XK_BackSpace,
  NS_VK_TAB,        XK_Tab,
  NS_VK_CLEAR,      XK_Clear,
  NS_VK_RETURN,     XK_Return,
  NS_VK_SHIFT,      XK_Shift_L,      
  NS_VK_SHIFT,      XK_Shift_R,      
  NS_VK_CONTROL,    XK_Control_L,
  NS_VK_CONTROL,    XK_Control_R,
  NS_VK_ALT,        XK_Alt_L,
  NS_VK_ALT,        XK_Alt_R,
  NS_VK_PAUSE,      XK_Pause,
  NS_VK_CAPS_LOCK,  XK_Caps_Lock,
  NS_VK_ESCAPE,     XK_Escape,
  NS_VK_SPACE,      XK_space,
  NS_VK_PAGE_UP,    XK_Page_Up,
  NS_VK_PAGE_DOWN,  XK_Page_Down,
  NS_VK_END,        XK_End,
  NS_VK_HOME,       XK_Home,
  NS_VK_LEFT,       XK_Left,
  NS_VK_UP,         XK_Up,
  NS_VK_RIGHT,      XK_Right,
  NS_VK_DOWN,       XK_Down, 
  NS_VK_PRINTSCREEN, XK_Print,
  NS_VK_INSERT,     XK_Insert,
  NS_VK_DELETE,     XK_Delete,

  NS_VK_NUMPAD0,    XK_KP_0, 
  NS_VK_NUMPAD1,    XK_KP_1,
  NS_VK_NUMPAD2,    XK_KP_2,
  NS_VK_NUMPAD3,    XK_KP_3,
  NS_VK_NUMPAD4,    XK_KP_4,
  NS_VK_NUMPAD5,    XK_KP_5,
  NS_VK_NUMPAD6,    XK_KP_6,
  NS_VK_NUMPAD7,    XK_KP_7,
  NS_VK_NUMPAD8,    XK_KP_8,
  NS_VK_NUMPAD9,    XK_KP_9,

  NS_VK_MULTIPLY,   XK_KP_Multiply,
  NS_VK_ADD,        XK_KP_Add,
  NS_VK_SEPARATOR,  XK_KP_Separator,
  NS_VK_SUBTRACT,   XK_KP_Subtract,
  NS_VK_DECIMAL,    XK_KP_Decimal,
  NS_VK_DIVIDE,     XK_KP_Divide,
  NS_VK_F1,         XK_F1,
  NS_VK_F2,         XK_F2,
  NS_VK_F3,         XK_F3,
  NS_VK_F4,         XK_F4,
  NS_VK_F5,         XK_F5,
  NS_VK_F6,         XK_F6,
  NS_VK_F7,         XK_F7,
  NS_VK_F8,         XK_F8,
  NS_VK_F9,         XK_F9,
  NS_VK_F10,        XK_F10,
  NS_VK_F11,        XK_F11,
  NS_VK_F12,        XK_F12,
  NS_VK_F13,        XK_F13,
  NS_VK_F14,        XK_F14,
  NS_VK_F15,        XK_F15,
  NS_VK_F16,        XK_F16,
  NS_VK_F17,        XK_F17,
  NS_VK_F18,        XK_F18,
  NS_VK_F19,        XK_F19,
  NS_VK_F20,        XK_F20,
  NS_VK_F21,        XK_F21,
  NS_VK_F22,        XK_F22,
  NS_VK_F23,        XK_F23,
  NS_VK_F24,        XK_F24,

  NS_VK_COMMA,      XK_comma,
  NS_VK_PERIOD,     XK_period, 
  NS_VK_SLASH,      XK_slash, 
//XXX: How do you get a BACK_QUOTE?  NS_VK_BACK_QUOTE, XK_backquote, 
  NS_VK_OPEN_BRACKET, XK_bracketleft, 
  NS_VK_CLOSE_BRACKET, XK_bracketright, 
  NS_VK_QUOTE, XK_quotedbl
  
}; 


int nsConvertKey(XID keysym)
{
 int i;
 int length = sizeof(nsKeycodes) / sizeof(struct nsKeyConverter);
 for (i = 0; i < length; i++) {
   if (nsKeycodes[i].keysym == keysym)
     return(nsKeycodes[i].vkCode);
 }

 return((int)keysym);
}

//==============================================================
void nsGtkWidget_InitNSEvent(GdkEvent *aGev,
                            gpointer   p,
                            nsGUIEvent &anEvent,
                            PRUint32   aEventType) 
{
  GdkEventButton *anXEv = (GdkEventButton*)aGev;

  anEvent.message = aEventType;
  anEvent.widget  = (nsWidget *) p;
  anEvent.eventStructType = NS_GUI_EVENT;

  if (anXEv != NULL) {
    anEvent.point.x = anXEv->x;
    anEvent.point.y = anXEv->y;
  }

  anEvent.time = anXEv->time;
}

//==============================================================
void nsGtkWidget_InitNSMouseEvent(GdkEvent *aGev,
                                 gpointer     p,
                                 nsMouseEvent &anEvent,
                                 PRUint32     aEventType) 
{
  GdkEventButton *anXEv = (GdkEventButton*)aGev;

  // Do base initialization
  nsGtkWidget_InitNSEvent(aGev, p, anEvent, aEventType);

  if (anXEv != NULL) { // Do Mouse Event specific intialization
    anEvent.time       = anXEv->time;
/*
    anEvent.isShift    = (anXEv->xbutton.state & ShiftMask) ? PR_TRUE : PR_FALSE;
    anEvent.isControl  = (anXEv->xbutton.state & ControlMask) ? PR_TRUE : PR_FALSE;
    anEvent.isAlt      = (anXEv->xbutton.state & Mod1Mask) ? PR_TRUE : PR_FALSE;
*/
    anEvent.clickCount = anXEv->button; //XXX Fix for double-clicks
    anEvent.eventStructType = NS_MOUSE_EVENT;

  }
}

//==============================================================
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define INTERSECTS(r1_x1,r1_x2,r1_y1,r1_y2,r2_x1,r2_x2,r2_y1,r2_y2) \
        !((r2_x2 <= r1_x1) ||\
          (r2_y2 <= r1_y1) ||\
          (r2_x1 >= r1_x2) ||\
          (r2_y1 >= r1_y2))

//==============================================================
typedef struct COLLAPSE_INFO {
    Window win;
    nsRect *r;
} CollapseInfo;

//==============================================================
#if 0
static Bool checkForExpose(Display *dpy, XEvent *evt, XtPointer client_data) 
{
    CollapseInfo *cinfo = (CollapseInfo*)client_data; 

    if ((evt->type == Expose && evt->xexpose.window == cinfo->win &&
         INTERSECTS(cinfo->r->x, cinfo->r->width, cinfo->r->y, cinfo->r->height,
                    evt->xexpose.x, evt->xexpose.y, 
                    evt->xexpose.x + evt->xexpose.width, 
                    evt->xexpose.y + evt->xexpose.height)) ||
         (evt->type == GraphicsExpose && evt->xgraphicsexpose.drawable == cinfo->win &&
         INTERSECTS(cinfo->r->x, cinfo->r->width, cinfo->r->y, cinfo->r->height,
                    evt->xgraphicsexpose.x, evt->xgraphicsexpose.y, 
                    evt->xgraphicsexpose.x + evt->xgraphicsexpose.width, 
                    evt->xgraphicsexpose.y + evt->xgraphicsexpose.height))) {

        return True;
    }
    return False;
}
#endif

//==============================================================
void nsGtkWidget_ExposureMask_EventHandler(GtkWidget *w, GdkEventExpose *event, gpointer p)
{
  nsWindow *widgetWindow = (nsWindow *)p;

  nsPaintEvent pevent;
  nsRect       rect;
  nsGtkWidget_InitNSEvent((GdkEvent*)event, p, pevent, NS_PAINT);
  pevent.rect = (nsRect *)&rect;
  XEvent xev;

  rect.x      = event->area.x;
  rect.y      = event->area.y;
  rect.width  = event->area.width;
  rect.height = event->area.height;


  if (widgetWindow->GetResized())
   return;

  if (event->type == GDK_NO_EXPOSE) {
    return;
  }
/* FIXME
  Display* display = XtDisplay(w);
  Window   window = XtWindow(w);

  XSync(display, FALSE);

  while (XCheckTypedWindowEvent(display, window, Expose, &xev) == TRUE) {
      rect.x      = xev.xexpose.x;
      rect.y      = xev.xexpose.y;
      rect.width  = xev.xexpose.width;
      rect.height = xev.xexpose.height;
  }
*/
  widgetWindow->OnPaint(pevent);
}

//==============================================================
void nsGtkWidget_ButtonPressMask_EventHandler(GtkWidget *w, GdkEvent * event, gpointer p)
{
  nsWindow *widgetWindow = (nsWindow *)p;
  nsMouseEvent mevent;
  nsGtkWidget_InitNSMouseEvent(event, p, mevent, NS_MOUSE_LEFT_BUTTON_DOWN);
  widgetWindow->DispatchMouseEvent(mevent);
}

//==============================================================
void nsGtkWidget_ButtonReleaseMask_EventHandler(GtkWidget *w, GdkEvent * event, gpointer p)
{
  nsWindow *widgetWindow = (nsWindow *)p;
  nsMouseEvent mevent;
  nsGtkWidget_InitNSMouseEvent(event, p, mevent, NS_MOUSE_LEFT_BUTTON_UP);
  widgetWindow->DispatchMouseEvent(mevent);
}

//==============================================================
void nsGtkWidget_ButtonMotionMask_EventHandler(GtkWidget *w, GdkEvent * event, gpointer p)
{
  nsPaintEvent pevent;
  nsWindow *widgetWindow = (nsWindow *)p;
  nsMouseEvent mevent;
  nsGtkWidget_InitNSMouseEvent(event, p, mevent, NS_MOUSE_MOVE);
  widgetWindow->DispatchMouseEvent(mevent);
}

//==============================================================
void nsGtkWidget_MotionMask_EventHandler(GtkWidget *w, GdkEvent * event, gpointer p)
{
  nsWindow *widgetWindow = (nsWindow *)p;
  nsMouseEvent mevent;
  nsGtkWidget_InitNSMouseEvent(event, p, mevent, NS_MOUSE_MOVE);
  widgetWindow->DispatchMouseEvent(mevent);
}

//==============================================================
void nsGtkWidget_EnterMask_EventHandler(GtkWidget *w, GdkEvent * event, gpointer p)
{
  nsWindow *widgetWindow = (nsWindow *)p;
  nsMouseEvent mevent;
  nsGtkWidget_InitNSMouseEvent(event, p, mevent, NS_MOUSE_ENTER);
  widgetWindow->DispatchMouseEvent(mevent);
}

//==============================================================
void nsGtkWidget_LeaveMask_EventHandler(GtkWidget *w, GdkEvent * event, gpointer p)
{
  if (DBG) fprintf(stderr, "***************** nsGtkWidget_LeaveMask_EventHandler\n");
  nsWindow * widgetWindow = (nsWindow *)p;
  nsMouseEvent mevent;
  nsGtkWidget_InitNSMouseEvent(event, p, mevent, NS_MOUSE_EXIT);
  widgetWindow->DispatchMouseEvent(mevent);
}

//==============================================================
void nsGtkWidget_Focus_Callback(GtkWidget *w, gpointer p, gpointer call_data)
{
#if 0
  nsWindow * widgetWindow = (nsWindow *) p ;

  XmAnyCallbackStruct * cbs = (XmAnyCallbackStruct*)call_data;
  nsGUIEvent event;
  nsGtkWidget_InitNSEvent(cbs->event, p, event, 
                         cbs->reason == XmCR_FOCUS?NS_GOTFOCUS:NS_LOSTFOCUS);
  widgetWindow->DispatchFocus(event);
#endif
}

//==============================================================
void nsGtkWidget_Toggle_Callback(GtkWidget *w, gpointer p, gpointer call_data)
{
#if 0
  nsWindow * widgetWindow = (nsWindow *) p ;
  if (DBG) fprintf(stderr, "***************** nsGtkWidget_Scrollbar_Callback\n");

  nsScrollbarEvent sevent;
  XmToggleButtonCallbackStruct * cbs = (XmToggleButtonCallbackStruct*)call_data;
#endif
}

//==============================================================
void nsGtkWidget_Toggle_ArmCallback(GtkWidget *w, gpointer p, gpointer call_data)
{
#if 0
  nsCheckButton * checkBtn = (nsCheckButton *) p ;

  XmToggleButtonCallbackStruct * cbs = (XmToggleButtonCallbackStruct*)call_data;
  
  if (DBG) fprintf(stderr, "Callback struct 0x%x\n", cbs);fflush(stderr);
  checkBtn->Armed();
#endif
}

//==============================================================
void nsGtkWidget_Toggle_DisArmCallback(GtkWidget *w, gpointer p, gpointer call_data)
{
#if 0
  nsCheckButton * checkBtn = (nsCheckButton *) p ;
  nsScrollbarEvent sevent;
  XmToggleButtonCallbackStruct * cbs = (XmToggleButtonCallbackStruct*)call_data;
  checkBtn->DisArmed();
#endif
}

//==============================================================
void nsGtkWidget_RadioButton_ArmCallback(GtkWidget *w, gpointer p, gpointer call_data)
{
#if 0
  nsRadioButton * radioBtn = (nsRadioButton *) p ;
  XmToggleButtonCallbackStruct * cbs = (XmToggleButtonCallbackStruct*)call_data;
  radioBtn->Armed();
  nsMouseEvent mevent;
  nsGtkWidget_InitNSMouseEvent(cbs->event, p, mevent, NS_MOUSE_LEFT_BUTTON_DOWN);
  radioBtn->DispatchMouseEvent(mevent);
#endif
}

//==============================================================
void nsGtkWidget_RadioButton_DisArmCallback(GtkWidget *w, gpointer p, gpointer call_data)
{
#if 0
  nsRadioButton * radioBtn = (nsRadioButton *) p ;
  nsScrollbarEvent sevent;
  XmToggleButtonCallbackStruct * cbs = (XmToggleButtonCallbackStruct*)call_data;
  radioBtn->DisArmed();
  nsMouseEvent mevent;
  nsGtkWidget_InitNSMouseEvent(cbs->event, p, mevent, NS_MOUSE_LEFT_BUTTON_UP);
  radioBtn->DispatchMouseEvent(mevent);
#endif
}


//==============================================================
void nsGtkWidget_Scrollbar_Callback(GtkWidget *w, gpointer p, gpointer call_data)
{
#if 0
  nsWindow * widgetWindow = (nsWindow *) p ;
  nsScrollbarEvent sevent;
  XmScrollBarCallbackStruct * cbs = (XmScrollBarCallbackStruct*) call_data;
  sevent.widget  = (nsWindow *) p;
  if (cbs->event != nsnull) {
    sevent.point.x = cbs->event->xbutton.x;
    sevent.point.y = cbs->event->xbutton.y;
  } else {
    sevent.point.x = 0;
    sevent.point.y = 0;
  }
  sevent.time    = 0; //XXX Implement this

  switch (cbs->reason) {

    case XmCR_INCREMENT:
      sevent.message = NS_SCROLLBAR_LINE_NEXT;
      break;

    case XmCR_DECREMENT:
      sevent.message = NS_SCROLLBAR_LINE_PREV;
      break;

    case XmCR_PAGE_INCREMENT:
      sevent.message = NS_SCROLLBAR_PAGE_NEXT;
      break;

    case XmCR_PAGE_DECREMENT:
      sevent.message = NS_SCROLLBAR_PAGE_PREV;
      break;

    case XmCR_DRAG:
      sevent.message = NS_SCROLLBAR_POS;
      break;

    case XmCR_VALUE_CHANGED:
      sevent.message = NS_SCROLLBAR_POS;
      break;

    default:
      break;
  }
  widgetWindow->OnScroll(sevent, cbs->value);
#endif
}



//==============================================================
void nsGtkWidget_Expose_Callback(GtkWidget *w, gpointer p, gpointer call_data)
{
#if 0
  nsWindow * widgetWindow = (nsWindow *) p ;
  if (widgetWindow == nsnull) {
    return;
  }

  XmDrawingAreaCallbackStruct * cbs = (XmDrawingAreaCallbackStruct *)call_data;
  XEvent * event = cbs->event;
  nsPaintEvent pevent;
  nsRect       rect;
  nsGtkWidget_InitNSEvent(event, p, pevent, NS_PAINT);
  pevent.rect = (nsRect *)&rect;
  widgetWindow->OnPaint(pevent);
#endif
}

//==============================================================
void nsGtkWidget_Resize_Callback(GtkWidget *w, gpointer p, gpointer call_data)
{
}

//==============================================================
void nsGtkWidget_Text_Callback(GtkWidget *w, gpointer p, gpointer call_data)
{
#if 0
  nsWindow * widgetWindow = (nsWindow *) p ;
  int len;
  XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *) call_data;
  PasswordData * data;
  XtVaGetValues(w, XmNuserData, &data, NULL);
  if (data == NULL || data->mIgnore) {
    return;
  }

  if (cbs->reason == XmCR_ACTIVATE) {
    printf ("Password: %s\n", data->mPassword.ToNewCString());
    return;
  }

  if (cbs->startPos < cbs->currInsert) {   /* backspace */
      cbs->endPos = data->mPassword.Length();  /* delete from here to end */
      data->mPassword.SetLength(cbs->startPos); /* backspace--terminate */
      return;
  }

  if (cbs->startPos == cbs->currInsert && cbs->currInsert < data->mPassword.Length()) {
    nsString insStr(cbs->text->ptr);
    data->mPassword.Insert(insStr, cbs->currInsert, strlen(cbs->text->ptr));
  } else if (cbs->startPos == cbs->currInsert && cbs->endPos != cbs->startPos) {
    data->mPassword.SetLength(cbs->startPos);
    printf("Setting Length [%s] at %d\n", cbs->text->ptr, cbs->currInsert);
  } else if (cbs->startPos == cbs->currInsert) {   /* backspace */
    data->mPassword.Append(cbs->text->ptr);
  } 
  
  for (len = 0; len < cbs->text->length; len++)
    cbs->text->ptr[len] = '*';
#endif
}

//==============================================================
void nsGtkWidget_FSBCancel_Callback(GtkWidget *w, gpointer p)
{
#if 0
  nsFileWidget * widgetWindow = (nsFileWidget *) p ;
  if (p != nsnull) {
    widgetWindow->OnCancel();
  }
#endif
}

//==============================================================
void nsGtkWidget_FSBOk_Callback(GtkWidget *w, gpointer p)
{
#if 0
  nsFileWidget * widgetWindow = (nsFileWidget *) p;
  if (p != nsnull) {
    widgetWindow->OnOk();
  }
#endif
}

//==============================================================
void nsGtkWidget_InitNSKeyEvent(int aEventType, nsKeyEvent& aKeyEvent,
                                GtkWidget *w, gpointer p, GdkEvent * event, gboolean * b)
{
#if 0
  nsWindow * widgetWindow = (nsWindow *) p ;
  Modifiers modout = 0;
  KeySym res;

  nsGtkWidget_InitNSEvent(event, p, aKeyEvent, aEventType);
  XKeyEvent* xKeyEvent =  (XKeyEvent*)event; 

   // Get the modout to test for shift + control
  XtTranslateKeycode(xKeyEvent->display,xKeyEvent->keycode, xKeyEvent->state, &modout, &res); 
  res = XKeycodeToKeysym(xKeyEvent->display, xKeyEvent->keycode, 0);

  aKeyEvent.keyCode   = nsConvertKey(res) & 0x00FF;
  aKeyEvent.time      = xKeyEvent->time; 
  aKeyEvent.isShift   = (xKeyEvent->state & ShiftMask) ? PR_TRUE : PR_FALSE; 
  aKeyEvent.isControl = (xKeyEvent->state & ControlMask) ? PR_TRUE : PR_FALSE;
  aKeyEvent.isAlt     = (xKeyEvent->state & Mod1Mask) ? PR_TRUE : PR_FALSE;
#endif
}

//==============================================================
void nsGtkWidget_KeyPressMask_EventHandler(GtkWidget *w, GdkEvent * event, gpointer p)
{
#if 0
  nsKeyEvent kevent;
  nsGtkWidget_InitNSKeyEvent(NS_KEY_DOWN, kevent, w, p, event, b);
  nsWindow * widgetWindow = (nsWindow *) p ;
  widgetWindow->OnKey(NS_KEY_DOWN, kevent.keyCode, &kevent);
#endif
}

//==============================================================
void nsGtkWidget_KeyReleaseMask_EventHandler(GtkWidget *w, GdkEvent * event, gpointer p)
{
#if 0
  nsKeyEvent kevent;
  nsGtkWidget_InitNSKeyEvent(NS_KEY_UP, kevent, w, p, event, b);
  nsWindow * widgetWindow = (nsWindow *) p ;
  widgetWindow->OnKey(NS_KEY_UP, kevent.keyCode, &kevent);
#endif
}

//==============================================================
void nsGtkWidget_ResetResize_Callback(gpointer call_data)
{
#if 0
    nsWindow* widgetWindow = (nsWindow*)call_data;
    widgetWindow->SetResized(PR_FALSE);
#endif
}

//==============================================================
void nsGtkWidget_Menu_Callback(GtkWidget *w, gpointer p, gpointer call_data)
{
#if 0
  nsIMenuItem * menuItem = (nsIMenuItem *)p;
  if (menuItem != NULL) {
    nsMenuEvent mevent;
    mevent.message = NS_MENU_SELECTED;
    mevent.eventStructType = NS_MENU_EVENT;
    mevent.point.x = 0;
    mevent.point.y = 0;
    menuItem->GetTarget(mevent.widget);
    menuItem->GetCommand(mevent.mCommand);
    mevent.mMenuItem = menuItem;
    mevent.time = 0; //XXX: Implement this
    nsEventStatus status;
    mevent.widget->DispatchEvent((nsGUIEvent *)&mevent, status);
  }
#endif
}

