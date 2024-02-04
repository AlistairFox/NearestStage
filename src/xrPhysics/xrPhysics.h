#pragma once

#ifdef XRPHYSICS_EXPORTS
#define XRPHYSICS_API
#else
#define XRPHYSICS_API
	#ifndef	_EDITOR
		#pragma comment( lib, "xrPhysics.lib"	)
	#else
		#pragma comment( lib, "xrPhysicsB.lib"	)
	#endif
#endif

