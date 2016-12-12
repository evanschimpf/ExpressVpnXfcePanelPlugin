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

#include <stdio.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-hvbox.h>

#include "express_vpn.h"

// Register the plugin
XFCE_PANEL_PLUGIN_REGISTER(express_vpn_construct);

// Temporary buffer used for executing commands
gchar temp_buf[BUFFER_SIZE];

// Executes a command and stores the result in the return buffer
static gint
execute_command(gchar *command,
                gchar *return_buffer,
                gint   return_buffer_size)
{
  // Working solution, read in one character at a time. Downside is that this
  // causes special characters (to change console color) to be read into the buffer
  FILE *fp;
  gint i;
  gint character;

  memset(return_buffer, 0, return_buffer_size);

  fp = popen(command, "r");
  if(fp == NULL)
    return -1;

  i = 0;
  while (i<return_buffer_size)
  {
    character = fgetc(fp);
    if(character == EOF)
      break;

    return_buffer[i++] = (gchar) character;
  }

  //g_message("Command:\n%s", command);
  //g_message("Returned:\n%s", return_buffer);

  return pclose(fp);
}

// Updates status for the plugin
static void
update_status(ExpressVpnPlugin *expressVpn)
{
  execute_command("expressvpn status", expressVpn->status, PATH_MAX);

  // Remove newline character from end of string
  gchar *temp = expressVpn->status;
  while(*temp != '\n')
    temp++;
  *temp = '\0';

  // Set status label with new status
  gtk_menu_item_set_label(GTK_MENU_ITEM(expressVpn->statusMenuItem),
                          expressVpn->status);
}

// Updates preferences for the plugin
static void
update_preferences(ExpressVpnPlugin *expressVpn)
{
  gchar *ptr;
  gchar first_char;

  // Read preferences from console
  execute_command("expressvpn preferences", temp_buf, BUFFER_SIZE);

  ptr = temp_buf;

  while(*ptr != '\0')
  {
    // Read first letter of character, will be used to determine which preference to set
    first_char = *ptr;

    // Advance to the value field
    while(*ptr != '\t')
      ptr++;
    while(*ptr == '\t')
      ptr++;

    // Set preference
    switch(first_char)
    {
      case 'a': // auto_connect       (true/false)
        expressVpn->autoConnect = (*ptr == 't');
        break;
      case 'p': // preferred_protocol (auto/tcp/udp)
        if(*ptr == 'a')
          expressVpn->protocol = EXPRESS_VPN_PROTOCOL_AUTO;
        else if(*ptr == 't')
          expressVpn->protocol = EXPRESS_VPN_PROTOCOL_TCP;
        else // (first_char == 'u')
          expressVpn->protocol = EXPRESS_VPN_PROTOCOL_UDP;
        break;
      case 's': // send_diagnostics   (true/false)
        expressVpn->sendDiagnostics = (*ptr == 't');
        break;
    }

    // Advance to the beginning of the next line
    next_line(ptr);
  }

  // Block signals on autoconnect menu items
  g_signal_handlers_block_by_func(G_OBJECT(expressVpn->autoConnectEnabledCheckMenuItem),
                                  auto_connect_enabled_menu_item_handler,
                                  expressVpn);
  g_signal_handlers_block_by_func(G_OBJECT(expressVpn->autoConnectDisabledCheckMenuItem),
                                  auto_connect_disabled_menu_item_handler,
                                  expressVpn);

  // Update autoconnect menu items
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(expressVpn->autoConnectEnabledCheckMenuItem),
                                 expressVpn->autoConnect);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(expressVpn->autoConnectDisabledCheckMenuItem),
                                 !expressVpn->autoConnect);

  // Unblock signals on menu items
  g_signal_handlers_unblock_by_func(G_OBJECT(expressVpn->autoConnectEnabledCheckMenuItem),
                                    auto_connect_enabled_menu_item_handler,
                                    expressVpn);
  g_signal_handlers_unblock_by_func(G_OBJECT(expressVpn->autoConnectDisabledCheckMenuItem),
                                    auto_connect_disabled_menu_item_handler,
                                    expressVpn);

  // Block signals on protocol menu items
  g_signal_handlers_block_by_func(G_OBJECT(expressVpn->protocolAutoCheckMenuItem),
                                  protocol_auto_menu_item_handler,
                                  expressVpn);
  g_signal_handlers_block_by_func(G_OBJECT(expressVpn->protocolTcpCheckMenuItem),
                                  protocol_tcp_menu_item_handler,
                                  expressVpn);
  g_signal_handlers_block_by_func(G_OBJECT(expressVpn->protocolUdpCheckMenuItem),
                                  protocol_udp_menu_item_handler,
                                  expressVpn);

  // Update protocol menu items
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(expressVpn->protocolAutoCheckMenuItem),
                                 (expressVpn->protocol == EXPRESS_VPN_PROTOCOL_AUTO));
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(expressVpn->protocolTcpCheckMenuItem),
                                 (expressVpn->protocol == EXPRESS_VPN_PROTOCOL_TCP));
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(expressVpn->protocolUdpCheckMenuItem),
                                 (expressVpn->protocol == EXPRESS_VPN_PROTOCOL_UDP));

  // Unblock signals on protocol menu items
  g_signal_handlers_unblock_by_func(G_OBJECT(expressVpn->protocolAutoCheckMenuItem),
                                    protocol_auto_menu_item_handler,
                                    expressVpn);
  g_signal_handlers_unblock_by_func(G_OBJECT(expressVpn->protocolTcpCheckMenuItem),
                                    protocol_tcp_menu_item_handler,
                                    expressVpn);
  g_signal_handlers_unblock_by_func(G_OBJECT(expressVpn->protocolUdpCheckMenuItem),
                                    protocol_udp_menu_item_handler,
                                    expressVpn);
}

// Updates the list of servers for the plugin
static void
update_servers(ExpressVpnPlugin *expressVpn)
{
  ExpressVpnServer *expressVpnServer;
  GtkWidget        *menuItem;
  gchar            *ptr;
  gchar            *temp_ptr;
  gchar            *previous_country;
  gint              i;
  gboolean          end_of_list;

  execute_command("expressvpn list", temp_buf, BUFFER_SIZE);

  ptr = temp_buf;
  previous_country = NULL;
  end_of_list = FALSE;

  // Advance until 's', first server listing will be "smart"
  while(*ptr != 's')
    ptr++;

  // Skip line containing "smart" (since we already have a Connect Smart menu item)
  next_line(ptr);

  // Parse servers
  while(TRUE)
  {
    expressVpnServer = g_new0(ExpressVpnServer, 1);

    // Copy alias into new struct
    for(i=0; (i<EXPRESS_VPN_ALIAS_SIZE) && (*ptr != '\t'); i++)
    {
      expressVpnServer->alias[i] = *ptr;
      ptr++;
    }

    // If there are is than one tab, no country name (use previous_country)
    if(*ptr == '\t' && *(ptr+1) == '\t')
    {
      temp_ptr = previous_country;
      // Copy country from previous_country
      for(i=0; (i<EXPRESS_VPN_COUNTRY_SIZE) && (*temp_ptr != '\t'); i++)
      {
        expressVpnServer->country[i] = *temp_ptr;
        temp_ptr++;
      }
    }
    else
    {
      skip_tabs(ptr);

      previous_country = ptr;

      // Copy country
      for(i=0; (i<EXPRESS_VPN_COUNTRY_SIZE) && (*ptr != '\t'); i++)
      {
        expressVpnServer->country[i] = *ptr;
        ptr++;
      }

      // Add menu item to country list
      menuItem = gtk_menu_item_new_with_label(expressVpnServer->country);
      gtk_widget_show(menuItem);
      gtk_menu_append(GTK_MENU(expressVpn->connectCountryMenu), menuItem);
      expressVpn->countryMenuItemList = g_list_prepend(expressVpn->countryMenuItemList,
                                                       menuItem);
    }

    skip_tabs(ptr);

    // Copy location
    for(i=0; (i<EXPRESS_VPN_LOCATION_SIZE) && (*ptr != '\t' && *ptr != '\0'); i++)
    {
      expressVpnServer->location[i] = *ptr;
      ptr++;
    }

    // Advance until 'Y', '\n', or '\0' is encountered
    while(*ptr != 'Y' && (*ptr != '\n' && ptr != '\0'))
      ptr++;

    // Set recommended
    if(*ptr == 'Y')
    {
      expressVpnServer->recommended = TRUE;
      while(*ptr != '\n')
        ptr++;
    }

    // Check to see if we've reached the end of the list
    if(*ptr == '\0' || *(ptr+1) == '\0')
      end_of_list = TRUE;

    g_message("Alias: %s, Country: %s, Location: %s, Recommended: %d",
              expressVpnServer->alias, expressVpnServer->country,
              expressVpnServer->location, expressVpnServer->recommended);

    // Add server to the list
    expressVpn->locationList = g_list_prepend(expressVpn->locationList,
                                              expressVpnServer);

    // Add menu item to location list
    menuItem = gtk_menu_item_new_with_label(expressVpnServer->location);
    gtk_widget_show(menuItem);
    gtk_widget_set_name (menuItem, expressVpnServer->alias);
    gtk_menu_append(GTK_MENU(expressVpn->connectLocationMenu), menuItem);
    g_signal_connect(G_OBJECT(menuItem), "activate",
                     G_CALLBACK(connect_location_item_handler), expressVpn);
    expressVpn->locationMenuItemList = g_list_prepend(expressVpn->locationMenuItemList,
                                                      menuItem);

    // Add menu item to recommended if necessary
    if(expressVpnServer->recommended) {
      menuItem = gtk_menu_item_new_with_label(expressVpnServer->location);
      gtk_widget_show(menuItem);
      gtk_widget_set_name (menuItem, expressVpnServer->alias);
      gtk_menu_append(GTK_MENU(expressVpn->connectRecommendedMenu), menuItem);
      g_signal_connect(G_OBJECT(menuItem), "activate",
                       G_CALLBACK(connect_location_item_handler), expressVpn);
      expressVpn->locationMenuItemList = g_list_prepend(expressVpn->recommendedMenuItemList,
                                                        menuItem);
    }


    // Break out of while true if reached end of list
    if(end_of_list)
      break;

    // Advance ptr to first character of following line
    ptr++;
  }

  // Reverse lists, they were prepended for runtime efficiency
  expressVpn->locationList = g_list_reverse(expressVpn->locationList);
  expressVpn->locationMenuItemList = g_list_reverse(expressVpn->locationMenuItemList);
  expressVpn->recommendedMenuItemList = g_list_reverse(expressVpn->recommendedMenuItemList);
  expressVpn->countryMenuItemList = g_list_reverse(expressVpn->countryMenuItemList);
}

// Updates the status and proferences for the plugin
static void
update(ExpressVpnPlugin *expressVpn)
{
  update_status(expressVpn);
  update_preferences(expressVpn);
}

// Handler for "activate" signal on refreshMenuItem
static void
refresh_menu_item_handler(GtkMenuItem *menuItem,
                          ExpressVpnPlugin *expressVpn)
{
  update_status(expressVpn);
  update_preferences(expressVpn);
  update_servers(expressVpn);
}

// Handler for "activate" signal on disconnectMenuItem
static void
disconnect_menu_item_handler(GtkMenuItem *menuItem,
                             ExpressVpnPlugin *expressVpn)
{
  execute_command("expressvpn disconnect", temp_buf, BUFFER_SIZE);
  update_status(expressVpn);
}

// Handler for "activate" signal on connectSmartMenuItem
static void
connect_smart_menu_item_handler(GtkMenuItem *menuItem,
                                ExpressVpnPlugin *expressVpn)
{
  execute_command("expressvpn connect smart", temp_buf, BUFFER_SIZE);
  update(expressVpn);
}

// Handler for "activate" signal on autoConnectEnabledCheckMenuItem
static void
auto_connect_enabled_menu_item_handler(GtkMenuItem *menuItem,
                                       ExpressVpnPlugin *expressVpn)
{
  execute_command("expressvpn autoconnect true", temp_buf, BUFFER_SIZE);
  update(expressVpn);
}

// Handler for "activate" signal on autoConnectDisabledCheckMenuItem
static void
auto_connect_disabled_menu_item_handler(GtkMenuItem *menuItem,
                                        ExpressVpnPlugin *expressVpn)
{
  execute_command("expressvpn autoconnect false", temp_buf, BUFFER_SIZE);
  update(expressVpn);
}

// Handler for "activate" signal on protocolAutoCheckMenuItem
static void
protocol_auto_menu_item_handler(GtkMenuItem *menuItem,
                                ExpressVpnPlugin *expressVpn)
{
  execute_command("expressvpn protocol auto", temp_buf, BUFFER_SIZE);
  update(expressVpn);
}

// Handler for "activate" signal on protocolTCPCheckMenuItem
static void
protocol_tcp_menu_item_handler(GtkMenuItem *menuItem,
                               ExpressVpnPlugin *expressVpn)
{
  execute_command("expressvpn protocol tcp", temp_buf, BUFFER_SIZE);
  update(expressVpn);
}

// Handler for "activate" signal on protocolUdpCheckMenuItem
static void
protocol_udp_menu_item_handler(GtkMenuItem *menuItem,
                               ExpressVpnPlugin *expressVpn)
{
  execute_command("expressvpn protocol udp", temp_buf, BUFFER_SIZE);
  update(expressVpn);
}

static void
connect_location_item_handler(GtkMenuItem *menuItem,
                              ExpressVpnPlugin *expressVpn)
{
  gchar temp[PATH_MAX];
  sprintf(temp, "expressvpn connect %s", gtk_widget_get_name(GTK_WIDGET(menuItem)));
  execute_command(temp, temp_buf, BUFFER_SIZE);
  update(expressVpn);
}

// Allocates memory and builds a new plugin
static ExpressVpnPlugin *
express_vpn_new(XfcePanelPlugin *plugin)
{
  ExpressVpnPlugin *expressVpn;
  GtkOrientation    orientation;
  GtkWidget        *seperatorMenuItem;

  // Allocate memory for the plugin structure
  expressVpn = panel_slice_new0(ExpressVpnPlugin);

  // Pointer to plugin
  expressVpn->plugin = plugin;

  // Get the current orientation
  orientation = xfce_panel_plugin_get_orientation(plugin);

  // Initialize ebox
  expressVpn->ebox = gtk_event_box_new();
  gtk_widget_show(expressVpn->ebox);

  // Initialize hbox
  expressVpn->hvbox = xfce_hvbox_new(orientation, FALSE, 2);
  gtk_widget_show(expressVpn->hvbox);

  // Add hvbox to ebox
  gtk_container_add(GTK_CONTAINER(expressVpn->ebox), expressVpn->hvbox);

  // Initialize icon
  expressVpn->icon = gtk_image_new_from_file(ICON_PATH);

  // Initialize menuBar
  expressVpn->menuBar = gtk_menu_bar_new();
  gtk_widget_show(expressVpn->menuBar);

  // Initialize menu (no need to call gtk_widget_show for menu)
  expressVpn->menu = gtk_menu_new();

  // Initialize imageMenuItem
  expressVpn->imageMenuItem = gtk_image_menu_item_new();
  gtk_widget_show(expressVpn->imageMenuItem);

  // Set image for imageMenuItem
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(expressVpn->imageMenuItem),
                                expressVpn->icon);

  // Set imageMenuItem to always show image since there is no label.
  gtk_image_menu_item_set_always_show_image(
    GTK_IMAGE_MENU_ITEM(expressVpn->imageMenuItem), TRUE);

  // Set menu as the submenu for imageMenuItem
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(expressVpn->imageMenuItem),
                            expressVpn->menu);

  // Append imageMenuItem to menuBar
  gtk_menu_bar_append(GTK_MENU_BAR(expressVpn->menuBar),
                      expressVpn->imageMenuItem);

  // Initialize statusMenuItem, add it to the main menu
  expressVpn->statusMenuItem = gtk_menu_item_new_with_label("Status");
  gtk_widget_show(expressVpn->statusMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->menu),expressVpn->statusMenuItem);
  gtk_widget_set_sensitive(expressVpn->statusMenuItem, FALSE);

  // Add a seperator to the menu
  seperatorMenuItem = gtk_separator_menu_item_new();
  gtk_widget_show(seperatorMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->menu),seperatorMenuItem);

  // Initialize refreshMenuItem, add it to the main menu
  expressVpn->refreshMenuItem = gtk_menu_item_new_with_label("Refresh");
  gtk_widget_show(expressVpn->refreshMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->menu), expressVpn->refreshMenuItem);
  g_signal_connect(G_OBJECT(expressVpn->refreshMenuItem), "activate",
                   G_CALLBACK(refresh_menu_item_handler), expressVpn);

  // Add another seperator to the menu
  seperatorMenuItem = gtk_separator_menu_item_new();
  gtk_widget_show(seperatorMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->menu), seperatorMenuItem);

  // Initialize connectMenuItem, add it to the main menu
  expressVpn->connectMenuItem = gtk_menu_item_new_with_label("Connect");
  gtk_widget_show(expressVpn->connectMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->menu), expressVpn->connectMenuItem);

  // Initialize connectMenu
  expressVpn->connectMenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(expressVpn->connectMenuItem),
                            expressVpn->connectMenu);

  /* ----- BEGIN connectMenu ----- */
  // Initialize connectSmartMenuItem, add it to the connectMenu
  expressVpn->connectSmartMenuItem = gtk_menu_item_new_with_label("Smart");
  gtk_widget_show(expressVpn->connectSmartMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->connectMenu),
                  expressVpn->connectSmartMenuItem);
  g_signal_connect(G_OBJECT(expressVpn->connectSmartMenuItem), "activate",
                   G_CALLBACK(connect_smart_menu_item_handler), expressVpn);

  // Initialize connectRecommendedMenuItem, add it to the connectMenu
  expressVpn->connectRecommendedMenuItem = gtk_menu_item_new_with_label("Recommended");
  gtk_widget_show(expressVpn->connectRecommendedMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->connectMenu),
                  expressVpn->connectRecommendedMenuItem);

  // Initialize connectRecommendedMenu
  expressVpn->connectRecommendedMenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(expressVpn->connectRecommendedMenuItem),
                            expressVpn->connectRecommendedMenu);

  /* ----- BEGIN connectRecommendedMenu ----- */
  /* ------ END connectRecommendedMenu ------ */

  // Initialize connectCountryMenuItem, add it to the connectMenu
  expressVpn->connectCountryMenuItem = gtk_menu_item_new_with_label("Country");
  gtk_widget_show(expressVpn->connectCountryMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->connectMenu),
                  expressVpn->connectCountryMenuItem);

  // Initialize connectCountryMenu
  expressVpn->connectCountryMenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(expressVpn->connectCountryMenuItem),
                            expressVpn->connectCountryMenu);

  /* ----- BEGIN connectCountryMenu ----- */
  /* ------ END connectCountryMenu ------ */

  // Initialize connectLocationMenuItem, add it to the connectMenu
  expressVpn->connectLocationMenuItem = gtk_menu_item_new_with_label("Location");
  gtk_widget_show(expressVpn->connectLocationMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->connectMenu),
                  expressVpn->connectLocationMenuItem);

  // Initialize connectLocationMenu
  expressVpn->connectLocationMenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(expressVpn->connectLocationMenuItem),
                            expressVpn->connectLocationMenu);

  /* ----- BEGIN connectLocationMenu ----- */
  /* ------ END connectLocationMenu ------ */
  /* ------ END connectMenu ------ */

  // Initialize disconnectMenuItem, add it to the main menu
  expressVpn->disconnectMenuItem = gtk_menu_item_new_with_label("Disconnect");
  gtk_widget_show(expressVpn->disconnectMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->menu),
                  expressVpn->disconnectMenuItem);
  g_signal_connect(G_OBJECT(expressVpn->disconnectMenuItem), "activate",
                   G_CALLBACK(disconnect_menu_item_handler), expressVpn);

  // Add another seperator to the menu
  seperatorMenuItem = gtk_separator_menu_item_new();
  gtk_widget_show(seperatorMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->menu),
                  seperatorMenuItem);

  // Initialize autoConnectMenuItem, add it to the main menu
  expressVpn->autoConnectMenuItem = gtk_menu_item_new_with_label("Auto-connect");
  gtk_widget_show(expressVpn->autoConnectMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->menu),
                  expressVpn->autoConnectMenuItem);

  // Initialize autoConnectMenu
  expressVpn->autoConnectMenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(expressVpn->autoConnectMenuItem),
                            expressVpn->autoConnectMenu);

  /* ----- BEGIN autoConnectMenu ----- */
  // Initialize autoConnectEnabledCheckMenuItem, add it to the autoConnectMenu
  expressVpn->autoConnectEnabledCheckMenuItem = gtk_check_menu_item_new_with_label("Enabled");
  gtk_widget_show(expressVpn->autoConnectEnabledCheckMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->autoConnectMenu),
                  expressVpn->autoConnectEnabledCheckMenuItem);
  g_signal_connect(G_OBJECT(expressVpn->autoConnectEnabledCheckMenuItem), "activate",
                   G_CALLBACK(auto_connect_enabled_menu_item_handler), expressVpn);

  // Initialize autoConnectDisabledCheckMenuItem, add it to the autoConnectMenu
  expressVpn->autoConnectDisabledCheckMenuItem = gtk_check_menu_item_new_with_label("Disabled");
  gtk_widget_show(expressVpn->autoConnectDisabledCheckMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->autoConnectMenu),
                  expressVpn->autoConnectDisabledCheckMenuItem);
  g_signal_connect(G_OBJECT(expressVpn->autoConnectDisabledCheckMenuItem), "activate",
                   G_CALLBACK(auto_connect_disabled_menu_item_handler), expressVpn);
  /* ------ END autoConnectMenu ------ */

  // Initialize protocolMenuItem, add it to the main menu
  expressVpn->protocolMenuItem = gtk_menu_item_new_with_label("Protocol");
  gtk_widget_show(expressVpn->protocolMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->menu),
                  expressVpn->protocolMenuItem);

  // Initialize protocolMenu
  expressVpn->protocolMenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(expressVpn->protocolMenuItem),
                            expressVpn->protocolMenu);

  /* ----- BEGIN protocolMenu ----- */
  // Initialize protocolAutoCheckMenuItem, add it to the protocolMenu
  expressVpn->protocolAutoCheckMenuItem = gtk_check_menu_item_new_with_label("Auto");
  gtk_widget_show(expressVpn->protocolAutoCheckMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->protocolMenu),
                  expressVpn->protocolAutoCheckMenuItem);
  g_signal_connect(G_OBJECT(expressVpn->protocolAutoCheckMenuItem), "activate",
                   G_CALLBACK(protocol_auto_menu_item_handler), expressVpn);

  // Initialize protocolTcpCheckMenuItem, add it to the protocolMenu
  expressVpn->protocolTcpCheckMenuItem = gtk_check_menu_item_new_with_label("TCP");
  gtk_widget_show(expressVpn->protocolTcpCheckMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->protocolMenu),
                  expressVpn->protocolTcpCheckMenuItem);
  g_signal_connect(G_OBJECT(expressVpn->protocolTcpCheckMenuItem), "activate",
                   G_CALLBACK(protocol_tcp_menu_item_handler), expressVpn);

  // Initialize protocolUdpCheckMenuItem, add it to the protocolMenu
  expressVpn->protocolUdpCheckMenuItem = gtk_check_menu_item_new_with_label("UDP");
  gtk_widget_show(expressVpn->protocolUdpCheckMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->protocolMenu),
                  expressVpn->protocolUdpCheckMenuItem);
  g_signal_connect(G_OBJECT(expressVpn->protocolUdpCheckMenuItem), "activate",
                   G_CALLBACK(protocol_udp_menu_item_handler), expressVpn);
  /* ------ END protocolMenu ------ */

  // Add another seperator to the menu
  seperatorMenuItem = gtk_separator_menu_item_new();
  gtk_widget_show(seperatorMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->menu),
                  seperatorMenuItem);

  // Initialize activateMenuItem, add it to the main menu
  expressVpn->activateMenuItem = gtk_menu_item_new_with_label("Activate");
  gtk_widget_show(expressVpn->activateMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->menu),
                  expressVpn->activateMenuItem);

  // Initialize resetMenuItem, add it to the main menu
  expressVpn->resetMenuItem = gtk_menu_item_new_with_label("Reset");
  gtk_widget_show(expressVpn->resetMenuItem);
  gtk_menu_append(GTK_MENU(expressVpn->menu),
                  expressVpn->resetMenuItem);

  ///////////////////////////////////////////////
  /////////    END OF MENU BUILDING     /////////
  ///////////////////////////////////////////////

  // Pack menuBar into hvbox
  gtk_box_pack_start(GTK_BOX(expressVpn->hvbox), expressVpn->menuBar,
                     FALSE, FALSE, 0);

  // Update
  update(expressVpn);
  update_servers(expressVpn);

  return expressVpn;
}

// Handler for "free-data" signal. Frees memory allocated for this plugin
static void
express_vpn_free(XfcePanelPlugin  *plugin,
                 ExpressVpnPlugin *expressVpn)
{
  // Destroy the panel widgets
  gtk_widget_destroy(expressVpn->hvbox);

  // Free list items
  g_list_free_full(expressVpn->locationList, g_free);
  g_list_free_full(expressVpn->countryList, g_free);
  g_list_free_full(expressVpn->locationMenuItemList, g_free);
  g_list_free_full(expressVpn->recommendedMenuItemList, g_free);
  g_list_free_full(expressVpn->countryMenuItemList, g_free);
  g_list_free_full(expressVpn->locationList, g_free);

  // Free the plugin structure
  panel_slice_free(ExpressVpnPlugin, expressVpn);
}

// Handler for "orientation-changed" signal
static void
express_vpn_orientation_changed(XfcePanelPlugin  *plugin,
                                GtkOrientation    orientation,
                                ExpressVpnPlugin *expressVpn)
{
  // change the orienation of the box
  xfce_hvbox_set_orientation(XFCE_HVBOX(expressVpn->hvbox), orientation);
}

// Handler for "size-changed" signal
static gboolean
express_vpn_size_changed(XfcePanelPlugin  *plugin,
                        gint              size,
                        ExpressVpnPlugin *expressVpn)
{
  GtkOrientation orientation;

  // Get the orientation of the plugin
  orientation = xfce_panel_plugin_get_orientation(plugin);

  // Set the widget size
  if(orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request(GTK_WIDGET(plugin), -1, size);
  else
    gtk_widget_set_size_request(GTK_WIDGET(plugin), size, -1);

  // We handled the orientation
  return TRUE;
}

// Handler for "about" signal. Shows about dialog
static void
express_vpn_about(XfcePanelPlugin *plugin)
{
  // About dialog code. You can use the GtkAboutDialog or the XfceAboutInto widget
}

// Constructor for plugin
static void
express_vpn_construct(XfcePanelPlugin *plugin)
{

  ExpressVpnPlugin *expressVpn;

  // Create the plugin
  expressVpn = express_vpn_new(plugin);

  // Add the ebox to the panel
  gtk_container_add(GTK_CONTAINER(plugin), expressVpn->ebox);

  // Show the panel's right click menu on the menubar
  xfce_panel_plugin_add_action_widget(plugin, expressVpn->menuBar);

  // Connect plugin signals
  g_signal_connect(G_OBJECT(plugin), "free-data",
                   G_CALLBACK(express_vpn_free), expressVpn);

  g_signal_connect(G_OBJECT(plugin), "size-changed",
                   G_CALLBACK(express_vpn_size_changed), expressVpn);

  g_signal_connect(G_OBJECT(plugin), "orientation-changed",
                   G_CALLBACK(express_vpn_orientation_changed), expressVpn);

  // Show the about menu item and connect signals
  xfce_panel_plugin_menu_show_about(plugin);
  g_signal_connect(G_OBJECT(plugin), "about",
                   G_CALLBACK(express_vpn_about), NULL);
}
