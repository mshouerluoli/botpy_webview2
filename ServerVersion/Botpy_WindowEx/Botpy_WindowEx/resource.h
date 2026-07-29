#pragma once

// Control IDs
#define IDC_STATUS_LABEL        1001
#define IDC_STATUS_DOT          1002
#define IDC_APPID_LABEL         1003
#define IDC_APPID_VALUE         1004
#define IDC_NICKNAME_LABEL      1005
#define IDC_NICKNAME_VALUE      1006
#define IDC_SESSIONID_LABEL     1007
#define IDC_SESSIONID_VALUE     1008
#define IDC_MSGCOUNT_LABEL      1009
#define IDC_MSGCOUNT_VALUE      1010
#define IDC_HEARTBEAT_LABEL     1011
#define IDC_HEARTBEAT_VALUE     1012
#define IDC_TAB_LOG             1013
#define IDC_TAB_PLUGIN          1014
#define IDC_LOG_EDIT            1015
#define IDC_TAB_CONTROL         1016
#define IDC_LEFT_PANEL          1017
#define IDC_RIGHT_PANEL         1018

// Plugin management control IDs
#define IDC_PLUGIN_LISTVIEW     1100
#define IDC_PLUGIN_REFRESH     1101
#define IDC_PLUGIN_ENABLE       1102
#define IDC_PLUGIN_DISABLE      1103
#define IDC_PLUGIN_UNLOAD       1104

// Custom window messages (for thread-safe communication)
#define WM_UI_LOG               (WM_APP + 1)
#define WM_UI_STATUS            (WM_APP + 2)
#define WM_UI_INFO              (WM_APP + 3)
#define WM_UI_MESSAGE_EVENT     (WM_APP + 4)
#define WM_UI_PLUGINS_UPDATE    (WM_APP + 5)
