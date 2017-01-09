/*

  Copyright (C) 2016 Evan Schimpf <evanschimpf@gmail.com>

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

#ifndef __EXPRESS_VPN_H__
#define __EXPRESS_VPN_H__

#define BUFFER_SIZE                 8192

#define EXPRESS_VPN_ALIAS_SIZE      8
#define EXPRESS_VPN_COUNTRY_SIZE    32
#define EXPRESS_VPN_LOCATION_SIZE   32

#define EXPRESS_VPN_PROTOCOL_AUTO   0
#define EXPRESS_VPN_PROTOCOL_TCP    1
#define EXPRESS_VPN_PROTOCOL_UDP    2

#define ICON_PATH                   "/home/evan/Projects/ExpressVpnXfcePanelPlugin/res/express_vpn_icon_16.png"

// Macro for skipping character while parsing string
#define skip_char(str, c)   while(*str == c) str++

#define skip_tabs(str)      skip_char(str, '\t')

#define next_line(str)      do { str++; } while(*(str-1) != '\n' && *str)
/*
-----------------------------------------
---------------- Structs ----------------
-----------------------------------------
*/

/*
  MENU LAYOUT

  status
  ------------
  refresh
  ------------
  connect
    smart
    recommended
    country
    location
  disconnect
  ------------
  auto-connect
    enabled
    disabled
  protocol
    auto
    udp
    tcp
  ------------
  activate
  reset
*/


typedef struct
{
  XfcePanelPlugin    *plugin;

  gchar               status[PATH_MAX];
  gint                protocol;
  gboolean            autoConnect;
  gboolean            sendDiagnostics;

  GtkWidget          *ebox;
  GtkWidget          *hvbox;

  GtkWidget          *menu;
  GtkWidget          *menuBar;

  GtkWidget          *imageMenuItem;
  GtkWidget          *icon;

  GtkWidget          *statusMenuItem;

  GtkWidget          *refreshMenuItem;

  GtkWidget          *connectMenuItem;
  GtkWidget          *connectMenu;
  GtkWidget          *connectSmartMenuItem;
  GtkWidget          *connectRecommendedMenuItem;
  GtkWidget          *connectRecommendedMenu;
  GtkWidget          *connectCountryMenuItem;
  GtkWidget          *connectCountryMenu;
  GtkWidget          *connectLocationMenuItem;
  GtkWidget          *connectLocationMenu;

  GtkWidget          *disconnectMenuItem;

  GtkWidget          *autoConnectMenuItem;
  GtkWidget          *autoConnectMenu;
  GtkWidget          *autoConnectEnabledCheckMenuItem;
  GtkWidget          *autoConnectDisabledCheckMenuItem;

  GtkWidget          *protocolMenuItem;
  GtkWidget          *protocolMenu;
  GtkWidget          *protocolAutoCheckMenuItem;
  GtkWidget          *protocolTcpCheckMenuItem;
  GtkWidget          *protocolUdpCheckMenuItem;


  GtkWidget          *activateMenuItem;
  GtkWidget          *resetMenuItem;

  GList              *locationMenuItemList;
  GList              *recommendedMenuItemList;
  GList              *countryMenuItemList;

  GList              *locationList;
  GList              *countryList;
}
ExpressVpnPlugin;

typedef struct
{
  gchar     alias[EXPRESS_VPN_ALIAS_SIZE];
  gchar     country[EXPRESS_VPN_COUNTRY_SIZE];
  gchar     location[EXPRESS_VPN_LOCATION_SIZE];
  gboolean  recommended;
}
ExpressVpnServer;

/*
-----------------------------------------
---------- Function Prototypes ----------
-----------------------------------------
*/
static gint
execute_command(gchar *command,
                gchar *return_buffer,
                gint   return_buffer_size);

static void
update_status(ExpressVpnPlugin *expressVpn);

static void
update_preferences(ExpressVpnPlugin *expressVpn);

static void
update_servers(ExpressVpnPlugin *expressVpn);

static void
update(ExpressVpnPlugin *expressVpn);

static ExpressVpnPlugin *
express_vpn_new(XfcePanelPlugin *plugin);

static void
express_vpn_free(XfcePanelPlugin  *plugin,
                 ExpressVpnPlugin *expressVpn);

static void
express_vpn_orientation_changed(XfcePanelPlugin  *plugin,
                                GtkOrientation    orientation,
                                ExpressVpnPlugin *expressVpn);

static gboolean
express_vpn_size_changed(XfcePanelPlugin  *plugin,
                         gint              size,
                         ExpressVpnPlugin *expressVpn);

static void
express_vpn_about(XfcePanelPlugin *plugin);

static void
express_vpn_construct(XfcePanelPlugin *plugin);

// Signal handlers
static void
refresh_menu_item_handler(GtkMenuItem *menuItem,
                          ExpressVpnPlugin *expressVpn);

static void
disconnect_menu_item_handler(GtkMenuItem *menuItem,
                             ExpressVpnPlugin *expressVpn);

static void
connect_smart_menu_item_handler(GtkMenuItem *menuItem,
                                ExpressVpnPlugin *expressVpn);

static void
connect_location_item_handler(GtkMenuItem *menuItem,
                              ExpressVpnPlugin *expressVpn);

static void
connect_country_item_handler(GtkMenuItem *menuItem,
                             ExpressVpnPlugin *expressVpn);

static void
auto_connect_menu_item_handler(GtkMenuItem *menuItem,
                               ExpressVpnPlugin *expressVpn);

static void
protocol_menu_item_handler(GtkMenuItem *menuItem,
                           ExpressVpnPlugin *expressVpn);

#endif
