#include "stdafx.h"
#include "xrServer.h"

void xrServer::Disconnect()
{

	script_server_events.clear();

	inherited::Disconnect	();
	SLS_Clear				();
	xr_delete				(game);
}
