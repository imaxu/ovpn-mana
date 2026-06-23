#pragma once
#include "ovpn-mana/ovpn_mana_api.h"

namespace ovpn::cli {

int handle_service_command(ovpn_mana_handle_t handle, int argc, char* argv[]);
int handle_client_command(ovpn_mana_handle_t handle, int argc, char* argv[]);
int handle_check_command();

}